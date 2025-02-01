#include "config.h"

#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <unordered_map>

#include "ubench/string.h"
#include "core_benchmark.h"
#include "corerw_benchmark.h"

static const std::unordered_map<std::string_view, cas_type>
    supported_cas_benchmarks = {
        {"default", cas_type::cpp},
#if defined(i386) || defined(__i386__) || defined(__i386)
        {"cas", cas_type::x86},
#endif
#if defined(__x86_64__)
        {"cas", cas_type::x86_64},
#endif
#if defined(__aarch64__)
        {"llsc", cas_type::arm64},
#endif
#if defined(__aarch64__) && HAVE_CXX_ARM64_LSE
        {"lse", cas_type::arm64_lse},
#endif
};

auto print_help(std::string_view prog_name) -> void {
  std::cout << "USAGE: " << prog_name
            << " [-s <samples>] [-i <iters>] [-b benchmark]" << std::endl;
  std::cout << std::endl;
  std::cout
      << "Execute Core Latency test for <iters> per <sample> for each core."
      << std::endl;
  std::cout << std::endl;
  std::cout << "Benchmarks supported are:" << std::endl;
  for (auto benchmark : supported_cas_benchmarks) {
    std::cout << " - " << benchmark.first << std::endl;
  }
  std::cout << " - readwrite" << std::endl;
}

auto main(int argc, char* argv[]) -> int {
  unsigned int samples = 500;
  unsigned int iterations = 4000;
  int c = 0;
  bool help = false;
  int exit_code = 0;
  std::string_view benchmark_str{};

  while ((c = getopt(argc, argv, "s:i:b:?")) != -1) {
    switch (c) {
      case 's': {
        auto samples_arg = ubench::string::parse_int<unsigned int>(optarg);
        if (samples_arg) {
          samples = *samples_arg;
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
        auto iters_arg = ubench::string::parse_int<unsigned int>(optarg);
        if (iters_arg) {
          iterations = *iters_arg;
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
      case 'b': {
        benchmark_str = optarg;
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

  std::unique_ptr<benchmark> bm{};
  if (benchmark_str == "readwrite") {
    bm = std::make_unique<corerw_benchmark>(iterations, samples);
  }
  if (!bm) {
    if (auto it = supported_cas_benchmarks.find(benchmark_str);
        it != supported_cas_benchmarks.end()) {
      bm = std::make_unique<core_benchmark>(iterations, samples, it->second);
    }
  }
  if (!bm) {
    if (benchmark_str.size() > 0) {
      std::cerr << "Error: benchmark type unknown." << std::endl;
      exit_code = 1;
      help = true;
    } else {
      bm = std::make_unique<core_benchmark>(iterations, samples, cas_type::cpp);
    }
  }

  if (help) {
    if (exit_code) std::cerr << std::endl;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    print_help(std::string_view(argv[0]));
    return exit_code;
  }

  if (!bm->init()) {
    std::cout << "Cannot initialise benchmark " << bm->name() << std::endl;
    std::cout << " - Check if your processor has the features required."
              << std::endl;
    return 1;
  }

  // Title with cores.
  std::cout << "Running " << bm->name() << " Core Benchmark" << std::endl;
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
        std::uint64_t time = bm->run(ping_core, pong_core);
        std::cout << std::left << std::setw(5) << time << " " << std::flush;
      }
    }
    std::cout << std::endl;
  }
  return 0;
}
