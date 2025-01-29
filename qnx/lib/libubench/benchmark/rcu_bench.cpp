#include <unistd.h>

#include <charconv>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string_view>
#include <thread>
#include <vector>

#include "ubench/atomics.h"

using namespace std::chrono_literals;

auto print_help(std::string_view prog_name) -> void {
  std::cout << "USAGE: " << prog_name << " [-p <threads>]" << std::endl;
  std::cout << std::endl;
  std::cout << "Execute RCU stress test for [-p] threads on the system"
            << std::endl;
}

auto run_stress(unsigned int threads) {
  std::cout << "Running with " << threads << " threads" << std::endl;

  std::unique_ptr data(std::make_unique<int>(42));
  rcu rcu(std::move(data));

  std::atomic<bool> terminate{false};
  std::atomic<std::uint32_t> counter{0};
  std::atomic<std::uint32_t> updates{0};

  std::thread thread_update([&]() {
    while (!terminate.load()) {
      std::this_thread::sleep_for(10ms);
      std::unique_ptr new_data(std::make_unique<int>(24));
      bool r = rcu.update(std::move(new_data));
      if (!r) {
        std::cout << "Error updating" << std::endl;
        std::abort();
      }
      updates++;
    }
  });

  std::vector<std::thread> reads;
  for (unsigned int t = 0; t < threads; t++) {
    reads.emplace_back([&]() {
      while (!terminate.load()) {
        rcu_ptr ptr = rcu.read();
        counter++;
      }
    });
  }

  std::this_thread::sleep_for(std::chrono::seconds(30));
  terminate.store(true);
  thread_update.join();
  for (std::thread& r : reads) {
    r.join();
  }

  std::cout << " Updates: " << updates << std::endl;
  std::cout << " Reads: " << counter << std::endl;
  std::cout << " Reads/core/sec: " << counter / threads / 30 << std::endl;
}

auto main(int argc, char* argv[]) -> int {
  int c = 0;
  unsigned int threads = 0;
  bool help = false;
  int exit_code = 0;

  while ((c = getopt(argc, argv, "p:?")) != -1) {
    switch (c) {
      case 'p': {
        std::string thread_string = optarg;
        auto [ptr, ec] = std::from_chars(thread_string.data(),
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            thread_string.data() + thread_string.size(), threads);
        if (ec == std::errc()) {
          if (threads < 1 || threads > std::thread::hardware_concurrency()) {
            std::cerr
                << "Error: Specify a minimum of 1 thread and not more than "
                << std::thread::hardware_concurrency() << " threads"
                << std::endl;
            exit_code = 1;
            help = true;
          }
        } else {
          std::cerr << "Error: Specify a thread count as a number" << std::endl;
          exit_code = 1;
          help = true;
        }
        break;
      }
      case '?':
        help = true;
        if (optopt) exit_code = 1;
        break;
      case ':':
        std::cerr << "Error: Option -" << optopt << " requires an operand"
                  << std::endl;
        exit_code = 1;
        help = true;
        break;
      default:
        std::cerr << "Error: Unknown option -" << optopt << std::endl;
        exit_code = 1;
        help = true;
        break;
    }
  }

  if (help) {
    if (exit_code) std::cerr << std::endl;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    print_help(std::string_view(argv[0]));
    return exit_code;
  }

  if (threads == 0) {
    threads = std::thread::hardware_concurrency();
  }

  run_stress(threads);
  return exit_code;
}
