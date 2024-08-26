#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <charconv>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <thread>

#include "core_benchmark.h"
#include "sync_event.h"

auto print_help(std::string_view prog_name) -> void {
  std::cout << "USAGE: " << prog_name << " [-s <samples>] [-i <iters>]"
            << std::endl;
  std::cout << std::endl;
  std::cout
      << "Execute Core Latency test for <iters> per <sample> for each core."
      << std::endl;
}

auto main(int argc, char* argv[]) -> int {
  int c = 0;
  unsigned int samples = 500;
  unsigned int iterations = 4000;
  bool help = false;
  int exit_code = 0;

  while ((c = getopt(argc, argv, "s:i:?")) != -1) {
    switch (c) {
      case 's': {
        std::string str_samples = optarg;
        auto [ptr, ec] = std::from_chars(
            str_samples.data(),
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            str_samples.data() + str_samples.size(), samples);
        if (ec == std::errc{}) {
          if (samples < 1 || samples > 60000) {
            std::cerr << "Error: Samples should be 1..60000" << std::endl;
            exit_code = 1;
            help = true;
          }
        } else {
          std::cerr << "Error: Specify a sample count as a number" << std::endl;
          exit_code = 1;
          help = true;
        }
        break;
      }
      case 'i': {
        std::string str_iters = optarg;
        auto [ptr, ec] = std::from_chars(
            str_iters.data(),
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            str_iters.data() + str_iters.size(), iterations);
        if (ec == std::errc{}) {
          if (iterations < 1 || iterations > 60000) {
            std::cerr << "Error: Iterations should be 1..60000" << std::endl;
            exit_code = 1;
            help = true;
          }
        } else {
          std::cerr << "Error: Specify iterations as a number" << std::endl;
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

  core_benchmark bm{iterations, samples};

  // Title with cores.
  std::cout << "Running Core Benchmark" << std::endl;
  std::cout << " Samples: " << samples << std::endl;
  std::cout << " Iterations: " << iterations << std::endl;
  std::cout << std::endl;

  std::cout << "      ";
  for (unsigned int pong_core = 0;
       pong_core < std::thread::hardware_concurrency(); pong_core++) {
    std::cout << std::left << std::setw(5) << pong_core << " ";
  }
  std::cout << std::endl;

  for (unsigned int ping_core = 0;
       ping_core < std::thread::hardware_concurrency(); ping_core++) {
    std::cout << std::left << std::setw(5) << ping_core << " ";
    for (unsigned int pong_core = 0;
         pong_core < std::thread::hardware_concurrency(); pong_core++) {
      if (pong_core == ping_core) {
        std::cout << "      " << std::flush;
      } else {
        std::uint64_t time = bm.run(ping_core, pong_core);
        std::cout << std::left << std::setw(5) << time << " " << std::flush;
      }
    }
    std::cout << std::endl;
  }
  return 0;
}