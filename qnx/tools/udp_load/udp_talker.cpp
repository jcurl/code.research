#include "udp_talker.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#ifndef NDEBUG
#include <sstream>
#endif

#include "ubench/measure/busy_measurement.h"

using namespace std::chrono_literals;

auto make_udp_talker(send_mode mode) -> std::unique_ptr<udp_talker> {
  std::unique_ptr<udp_talker> talker;
  switch (mode) {
    case send_mode::mode_sendto:
      talker = std::make_unique<udp_talker_sendto>();
      break;
    case send_mode::mode_sendmmsg:
      talker = std::make_unique<udp_talker_sendmmsg>();
      break;
    case send_mode::mode_bpf:
      talker = std::make_unique<udp_talker_bpf>();
      break;
    default:
      talker = std::make_unique<udp_talker>();
      break;
  }
  return talker;
}

auto idle_test(std::chrono::milliseconds duration) noexcept
    -> std::optional<ubench::measure::busy_measurement> {
  if (!ubench::chrono::idle_clock::is_available()) return {};
  std::this_thread::sleep_for(500ms);
  {
    ubench::measure::busy_stop_watch measure{};
    std::this_thread::sleep_for(duration);

    auto idle_measurement = measure.measure();
    return idle_measurement;
  }
}

auto udp_talker::set_source_addr(const struct sockaddr_in& addr) noexcept
    -> bool {
  if (addr.sin_family != AF_INET) return false;
  if (addr.sin_port == 0) return false;
  if (addr.sin_addr.s_addr == 0) return false;

  memset(&source_, 0, sizeof(source_));
  source_.sin_family = AF_INET;
  source_.sin_port = addr.sin_port;
  source_.sin_addr = addr.sin_addr;
  return true;
}

auto udp_talker::set_dest_addr(const struct sockaddr_in& addr) noexcept
    -> bool {
  if (addr.sin_family != AF_INET) return false;
  if (addr.sin_port == 0) return false;
  if (addr.sin_addr.s_addr == 0) return false;

  memset(&dest_, 0, sizeof(dest_));
  dest_.sin_family = AF_INET;
  dest_.sin_port = addr.sin_port;
  dest_.sin_addr = addr.sin_addr;
  return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto udp_talker::set_shaping(std::uint16_t slots, std::uint16_t width,
    std::uint32_t packets, std::uint16_t pkt_size) noexcept -> bool {
  if (slots < 1) return false;
  if (width < 1) return false;
  if (packets < 1) return false;
  if (pkt_size < 1) return false;

  slots_ = slots;
  width_ = width;
  packets_ = packets;
  size_ = pkt_size;
  return true;
}

auto udp_talker::init_packets(
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    [[maybe_unused]] const struct sockaddr_in& source,
    [[maybe_unused]] const struct sockaddr_in& dest,
    [[maybe_unused]] std::uint16_t pkt_size) noexcept -> bool {
  return false;
}

auto udp_talker::send_packets([[maybe_unused]] std::uint16_t count) noexcept
    -> std::uint16_t {
  return 0;
}

auto udp_talker::delay(std::chrono::nanoseconds ns) noexcept -> bool {
  if (delay_count_ > 5) return false;

  timespec ts = {ns.count() / 1000000000, ns.count() % 1000000000};
  timespec rm{};

  if (delay_count_ == 0) {
    std::cout << "." << std::flush;
  }
  delay_count_++;

  do {
#if HAVE_CLOCK_REALTIME_PRECISE
    int r = clock_nanosleep(CLOCK_REALTIME_PRECISE, 0, &ts, &rm);
#elif HAVE_CLOCK_REALTIME
    int r = clock_nanosleep(CLOCK_REALTIME, 0, &ts, &rm);
#else
    int r = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &rm);
#endif
    if (r == -1) {
      if (errno != EINTR) return false;
      ts = rm;
    }
  } while (rm.tv_sec > 0 || rm.tv_nsec > 0);
  return true;
}

auto udp_talker::init() noexcept -> bool {
  if (init_) return true;

  if (slots_ == 0 || width_ == 0 || size_ == 0 || packets_ == 0) return false;
  if (!init_packets(source_, dest_, size_)) return false;

  init_ = true;
  return true;
}

auto udp_talker::run(std::chrono::milliseconds duration) noexcept
    -> std::optional<udp_results> {
  if (duration.count() < 0) return {};
  if (!init()) return {};

#ifndef NDEBUG
  std::stringstream log;
#endif

  std::uint32_t r = 0;
  std::uint32_t s = 0;
  std::uint32_t packet_sent_window_count = 0;
  std::uint32_t sent = 0;
  std::uint32_t total_sent = 0;

  std::int64_t expected_time = std::max<std::int64_t>(
      duration.count(), static_cast<std::int64_t>(slots_) * width_);
  std::int64_t elapsed_time = 0;
  std::chrono::high_resolution_clock::duration total_sent_time{0};
  std::chrono::high_resolution_clock::duration total_wait_time{0};
  {
    auto start_time = std::chrono::high_resolution_clock::now();

    do {
      std::uint32_t packet_sent_expected = 0;
      if (s < slots_) {
        packet_sent_expected = packets_ * (s + 1) / slots_;
      } else {
        packet_sent_expected = packets_;
      }

      bool sending = true;
      while (sending) {
        std::uint32_t packet_sent_remaining =
            packet_sent_expected - packet_sent_window_count;

        // We shouldn't send more than this group at a time (especially if we're
        // sending multiple packets with one system call).
        //
        // Note: Do not refactor this equation because shaping is depending on
        // integer rounding.
        std::uint32_t packet_send_max =
            packets_ * (r + 1) / slots_ - packets_ * r / slots_;
        std::uint32_t packet_send = 0;
        if (packet_sent_remaining > packet_send_max) {
          packet_send = packet_send_max;
        } else {
          packet_send = packet_sent_remaining;
        }

        auto send_time = std::chrono::high_resolution_clock::now();
#ifndef NDEBUG
        auto send_time_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                send_time - start_time);
        log << "t=" << send_time_ms.count() << "; s=" << s << "; r=" << r
            << "; pse=" << packet_sent_expected
            << "; pswc=" << packet_sent_window_count
            << "; psr=" << packet_sent_remaining << "; ps=" << packet_send;
#endif
        sent = send_packets(packet_send);
        auto send_snapshot = std::chrono::high_resolution_clock::now();
        total_sent_time += (send_snapshot - send_time);
        elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            send_snapshot - start_time)
                           .count();
        packet_sent_window_count += sent;
        total_sent += sent;
        r++;

        std::uint32_t sf = elapsed_time / width_;
        std::int64_t W = 0;
        std::uint32_t sp = s;
#ifndef NDEBUG
        log << "; t'=" << elapsed_time << "; sent=" << sent
            << "; pswc'=" << packet_sent_window_count << "; sf=" << sf;
#endif
        if (sf <= s) {
          if (packet_sent_expected > packet_sent_window_count) {
            // Continue sending more groups within this slot.
#ifndef NDEBUG
            log << "  Resend!";
            std::cout << log.str() << std::endl;
            log.str(std::string());
#endif
            continue;
          }

          s++;
          W = static_cast<std::int64_t>(s * width_) - elapsed_time;
        } else {
          s = sf;
        }

        sending = false;
#ifndef NDEBUG
        log << "; W=" << W;
#endif

        // Increase credits for the upcoming slot from `sp+1` up to `s`
        // inclusive.
        if (s >= slots_ && packet_sent_window_count >= packets_) {
          // Only subtract credits once we've used the time of the full window.
          if (sp < slots_ - 1U) sp = slots_ - 1U;

          std::uint32_t slots_missed = s - sp;
          std::uint32_t credits =
              packets_ * (r + slots_missed) / slots_ - packets_ * r / slots_;
          if (packet_sent_window_count > credits) {
            packet_sent_window_count -= credits;
          } else {
#ifndef NDEBUG
            credits = packet_sent_window_count;
#endif
            packet_sent_window_count = 0;
          }
#ifndef NDEBUG
          log << "; missed=" << (slots_missed - 1U) << "; credits=" << credits;
#endif
        }

#ifndef NDEBUG
        std::cout << log.str() << std::endl;
        log.str(std::string());
#endif

        if (W >= 4) {
          std::this_thread::sleep_for(W * 1ms);
          auto wait_snapshot = std::chrono::high_resolution_clock::now();
          total_wait_time += wait_snapshot - send_snapshot;
        }

        // Error handling in routines, reset counter at the end of each
        // interval.
        delay_count_ = 0;
      }
    } while (elapsed_time < expected_time);
  }

  udp_results results = {
      total_sent,
      packets_ * s / slots_,
      std::chrono::duration_cast<std::chrono::milliseconds>(total_wait_time),
      std::chrono::duration_cast<std::chrono::milliseconds>(total_sent_time),
  };
  return results;
}
