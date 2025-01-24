#include <algorithm>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include "ubench/net.h"
#include "ubench/thread.h"
#include "options.h"
#include "udp_talker.h"

auto main(int argc, char* argv[]) -> int {
  // Initialisation.

  options options(argc, argv);
  if (!options.is_valid()) return 1;

  struct sockaddr_in saddr = options.source_addr();
  if (saddr.sin_port == 0) saddr.sin_port = htons(3499);
  struct sockaddr_in daddr = options.dest_addr();
  if (daddr.sin_port == 0) daddr.sin_port = htons(3500);

  std::uint32_t packets =
      std::max<std::uint32_t>(options.packets(), options.threads());

  // Prepare all the threads with the benchmarks.

  std::atomic<std::uint64_t> packets_sent{0};
  std::atomic<std::uint64_t> packets_expected{0};
  std::atomic<std::uint64_t> wait_time{0};
  std::atomic<std::uint64_t> sent_time{0};
  std::atomic<std::uint32_t> thread_failures{0};

  ubench::thread::sync_event sync{};
  std::vector<std::thread> runners{};
  for (auto i = 0; i < options.threads(); i++) {
    std::uint32_t packets_thread =
        (i + 1) * packets / options.threads() - i * packets / options.threads();

    // The options has checked beforehand if the talker mode makes sense or not,
    // so we don't check if it is supported again here.
    std::unique_ptr<udp_talker> talker{};
    switch (options.mode()) {
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
        std::cerr << "Error: Unknown sending mode" << std::endl;
        return 1;
    }

    talker->set_shaping(
        options.slots(), options.width(), packets_thread, options.size());
    if (!talker->set_source_addr(saddr)) {
      std::cerr << "Error: Invalid source address" << std::endl;
      return 1;
    }
    if (!talker->set_dest_addr(daddr)) {
      std::cerr << "Error: Invalid destination address" << std::endl;
      return 1;
    }
    if (!talker->init()) {
      std::cerr << "Error: Couldn't initialise talker" << std::endl;
      return 1;
    }

    auto runner = [&sync, &packets_sent, &packets_expected, &wait_time,
                      &sent_time,
                      &thread_failures](std::unique_ptr<udp_talker> talker,
                      std::chrono::milliseconds duration) {
      sync.wait();
      auto result = talker->run(duration);
      if (result) {
        packets_sent += result->packets_sent;
        packets_expected += result->packets_expected;
        wait_time += result->wait_time.count();
        sent_time += result->send_time.count();
      } else {
        thread_failures++;
      }
    };

    // Starts the thread.
    runners.emplace_back(runner, std::move(talker), options.duration());
  }

  // Diagnostics to the User.

  std::cout << "UDP Talker Parameters:" << std::endl;
  std::cout << " Mode: ";
  switch (options.mode()) {
    case send_mode::mode_sendto:
      std::cout << "sendto" << std::endl;
      break;
    case send_mode::mode_sendmmsg:
      std::cout << "sendmmsg" << std::endl;
      break;
    case send_mode::mode_bpf:
      std::cout << "bpf" << std::endl;
      break;
    default:
      std::cout << "unknown" << std::endl;
      break;
  }
  std::cout << " Source: " << saddr << std::endl;
  std::cout << " Destination: " << daddr << std::endl;
  std::cout << " Slots (n): " << options.slots() << std::endl;
  std::cout << " Width (m): " << options.width() << " (ms)" << std::endl;
  std::cout << " Packets (p): " << packets << std::endl;
  std::cout << " Size (s): " << options.size() << std::endl;
  std::cout << " Threads: " << options.threads() << std::endl;

  std::uint32_t t = options.slots() * options.width();
  std::cout << " Packets/sec: " << packets * 1000 / t << std::endl;
  std::cout << std::endl;

  // Run optional IDLE test.

  std::optional<busy_measurement> idle_measurement{};
  if (options.enable_idle_test()) {
    std::cout << "Performing IDLE test for 5s... " << std::flush;
    idle_measurement = idle_test(std::chrono::milliseconds(5000));
    if (idle_measurement) {
      std::cout << "done." << std::endl;
    } else {
      std::cout << "failed." << std::endl;
    }
  }

  // Run UDP test.

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  std::cout << "Performing TEST... " << std::flush;
  busy_measurement run_measurement{};
  {
    busy_stop_watch run{};
    sync.set();
    for (auto& t : runners) {
      t.join();
    }
    run_measurement = run.measure();
  }

  // Print results.

  if (thread_failures > 0) {
    std::cout << "failed in " << thread_failures << " threads." << std::endl;
    return 2;
  } else {
    std::cout << "done" << std::endl;
  }

  // Calculate busy time in units of 0.01%. The busy time is the total time
  // over all cores, which is why we must divide the result by the number of
  // cores, to get the system load.
  if (options.enable_idle_test() && idle_measurement) {
    std::uint32_t baseline = idle_measurement->busy_time.count() * 10000 /
                             idle_measurement->run_time.count();
    std::cout << "Total CPU Busy (Idle Test): " << baseline / 100 << "."
              << std::setw(2) << std::setfill('0') << baseline % 100 << "%"
              << std::endl;
  }

  std::uint32_t runtime = run_measurement.busy_time.count() * 10000 /
                          run_measurement.run_time.count();
  std::cout << "Total CPU Busy: " << runtime / 100 << "." << std::setw(2)
            << std::setfill('0') << runtime % 100 << "%" << std::endl;
  std::uint32_t cputime = run_measurement.cpu_time.count() * 10000 /
                          run_measurement.run_time.count();
  std::cout << "Process CPU Busy: " << cputime / 100 << "." << std::setw(2)
            << std::setfill('0') << cputime % 100 << "%" << std::endl;

  std::cout << "Time in send: " << sent_time << "ms" << std::endl;
  std::cout << "Time in sleep: " << wait_time << "ms" << std::endl;
  std::cout << "Packets Sent: " << packets_sent << std::endl;
  std::cout << "Packets Expected: " << packets_expected << std::endl;
  return 0;
}
