#include <cerrno>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>
#include <thread>

// NOLINTNEXTLINE(modernize-deprecated-headers)
#include <time.h>

#if __QNXNTO__
#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <inttypes.h>
#endif

#include "stdext/expected.h"

template <class TClock>
auto print_clock_details(std::string_view name) -> void {
  std::cout << std::left << std::setw(25) << name << "  " << std::left
            << std::setw(6) << (TClock::is_steady ? "Yes" : "No") << "  "
            << (TClock::period::num) << "/" << (TClock::period::den)
            << std::endl;
}

template <class TClock, class TDuration>
auto print_time(std::chrono::time_point<TClock, TDuration>& time)
    -> std::string {
  std::uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
      time.time_since_epoch())
                         .count();

  std::stringstream ss;
  ss << ns / 1000000000 << "." << std::right << std::setw(9)
     << std::setfill('0') << ns % 1000000000;
  return ss.str();
}

#if __QNXNTO__
#define QTIME_FLAG_STRING(x) \
  "  [" << ((SYSPAGE_ENTRY(qtime)->flags & x) ? 'X' : '-') << "] " #x

auto print_qnx_details() -> void {
  std::cout << "QNX Clock Details (pidin syspage=qtime)" << std::endl;
  std::cout << "- cycles_per_sec:  " << SYSPAGE_ENTRY(qtime)->cycles_per_sec
            << std::endl;
  std::cout << "- nsec_tod_adjust: " << SYSPAGE_ENTRY(qtime)->nsec_tod_adjust
            << std::endl;
#if !defined(__QNX__) || __QNX__ < 800
  std::cout << "- nsec:            " << SYSPAGE_ENTRY(qtime)->nsec << std::endl;
#endif
  std::cout << "- nsec_inc:        " << SYSPAGE_ENTRY(qtime)->nsec_inc
            << std::endl;
  std::cout << "- boot_time:       " << SYSPAGE_ENTRY(qtime)->boot_time
            << std::endl;
  std::cout << "- adjust:          "
            << "tick_nsec_inc=" << SYSPAGE_ENTRY(qtime)->adjust.tick_nsec_inc
            << "; tick_count=" << SYSPAGE_ENTRY(qtime)->adjust.tick_count
            << std::endl;
  std::cout << "- interrupt:       " << SYSPAGE_ENTRY(qtime)->intr << std::endl;
#if !defined(__QNX__) || __QNX__ < 800
  std::cout << "- timer_prog_time: " << SYSPAGE_ENTRY(qtime)->timer_prog_time
            << std::endl;
#endif
#if __QNX__ >= 800
  std::cout << "- boot_cc:         " << SYSPAGE_ENTRY(qtime)->boot_cc
            << std::endl;
  std::cout << "- tick_period_cc:  " << SYSPAGE_ENTRY(qtime)->tick_period_cc
            << std::endl;
#endif

  std::cout << "- Flags" << std::endl;
#if !defined(__QNX__) || __QNX__ < 800
  std::cout << QTIME_FLAG_STRING(QTIME_FLAG_TIMER_ON_CPU0) << std::endl;
#endif
  std::cout << QTIME_FLAG_STRING(QTIME_FLAG_CHECK_STABLE) << std::endl;
  std::cout << QTIME_FLAG_STRING(QTIME_FLAG_TICKLESS) << std::endl;
#if !defined(__QNX__) || __QNX__ < 800
  std::cout << QTIME_FLAG_STRING(QTIME_FLAG_TIMECC) << std::endl;
#endif
  std::cout << QTIME_FLAG_STRING(QTIME_FLAG_GLOBAL_CLOCKCYCLES) << std::endl;
}

auto qnx_clockcycles()
    -> std::chrono::time_point<std::chrono::high_resolution_clock,
        std::chrono::nanoseconds> {
  uint64_t c = ClockCycles();

  uint64_t cs = c / SYSPAGE_ENTRY(qtime)->cycles_per_sec;
  uint64_t cn = c % SYSPAGE_ENTRY(qtime)->cycles_per_sec * 1000000000 /
                SYSPAGE_ENTRY(qtime)->cycles_per_sec;
  auto duration = std::chrono::seconds{cs} + std::chrono::nanoseconds{cn};
  return std::chrono::time_point<std::chrono::high_resolution_clock,
      std::chrono::nanoseconds>{
      std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(
          duration)};
}
#endif

auto clock(clockid_t clockid)
    -> stdext::expected<std::chrono::time_point<std::chrono::system_clock,
                            std::chrono::nanoseconds>,
        int> {
  struct timespec tp = {};
  int result = clock_gettime(clockid, &tp);
  if (result) return stdext::unexpected{errno};

  auto duration =
      std::chrono::seconds{tp.tv_sec} + std::chrono::nanoseconds{tp.tv_nsec};
  return std::chrono::time_point<std::chrono::system_clock,
      std::chrono::nanoseconds>{
      std::chrono::duration_cast<std::chrono::system_clock::duration>(
          duration)};
}

auto main(/* int argc, char* argv[] */) -> int {
  std::cout << "Time Comparison Tool" << std::endl << std::endl;

  std::cout << "Clock                      Steady  Period" << std::endl;
  std::cout << "-------------------------  ------  ------" << std::endl;
  print_clock_details<std::chrono::high_resolution_clock>(
      "High Resolution Clock");
  print_clock_details<std::chrono::steady_clock>("Steady Clock");
  print_clock_details<std::chrono::system_clock>("System Clock");
  std::cout << std::endl;

#if __QNXNTO__
  print_qnx_details();
  std::cout << std::endl;
#endif

  std::cout << std::left << std::setw(22) << "System Clock"
            << " " << std::left << std::setw(22) << "Highres Clock"
            << " " << std::left << std::setw(22) << "Steady Clock"
            << " " << std::left << std::setw(22) << "CLOCK_MONOTONIC"
            << " " << std::left << std::setw(22) << "CLOCK_REALTIME"
#if __QNXNTO__
            << " " << std::left << std::setw(22) << "ClockCycles()"
#endif
            << " " << std::endl;
  std::cout << std::setfill('-') << std::left << std::setw(22) << ""
            << " " << std::left << std::setw(22) << ""
            << " " << std::left << std::setw(22) << ""
            << " " << std::left << std::setw(22) << ""
            << " " << std::left << std::setw(22) << ""
#if __QNXNTO__
            << " " << std::left << std::setw(22) << ""
#endif
            << " " << std::setfill(' ') << std::endl;

  for (;;) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto sysclk = std::chrono::system_clock::now();
    auto hiresclk = std::chrono::high_resolution_clock::now();
    auto stdclk = std::chrono::steady_clock::now();
    auto monoclk = clock(CLOCK_MONOTONIC);
    auto realclk = clock(CLOCK_REALTIME);
#if __QNXNTO__
    auto clkcyc = qnx_clockcycles();
#endif

    std::cout << std::right << std::setw(22) << print_time(sysclk) << " "
              << std::right << std::setw(22) << print_time(hiresclk) << " "
              << std::right << std::setw(22) << print_time(stdclk) << " ";
    if (monoclk) {
      std::cout << std::right << std::setw(22) << print_time(monoclk.value())
                << " ";
    } else {
      std::cout << std::right << std::setw(22) << "Error"
                << " ";
    }
    if (realclk) {
      std::cout << std::right << std::setw(22) << print_time(realclk.value())
                << " ";
    } else {
      std::cout << std::right << std::setw(22) << "Error"
                << " ";
    }
#if __QNXNTO__
    std::cout << std::right << std::setw(22) << print_time(clkcyc) << " ";
#endif
    std::cout << std::endl;
  }

  return 0;
}
