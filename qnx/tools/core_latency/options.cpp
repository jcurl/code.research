#include "options.h"

#include <unistd.h>

#include <iostream>

#include "stdext/expected.h"
#include "ubench/string.h"
#include "core_benchmark.h"

namespace {

auto print_help(std::string_view prog_name) -> void {
  std::cout << "USAGE: " << prog_name
            << " [-s <samples>] [-i <iters>] [-b benchmark]" << std::endl;
  std::cout << std::endl;
  std::cout
      << "Execute Core Latency test for <iters> per <sample> for each core."
      << std::endl;
  std::cout << std::endl;
  std::cout << "Benchmarks supported are:" << std::endl;
  for (auto benchmark : core_benchmark::supported()) {
    std::cout << " - " << benchmark.first << std::endl;
  }
  std::cout << " - readwrite" << std::endl;
}

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
auto make_options(int& argc, char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  int c = 0;
  bool help = false;
  int err = 0;
  options o{};

  while ((c = getopt(argc, argv, "s:i:b:?")) != -1) {
    switch (c) {
      case 's': {
        auto samples_arg = ubench::string::parse_int<unsigned int>(optarg);
        if (samples_arg) {
          o.samples_ = *samples_arg;
          if (o.samples_ < 1 || o.samples_ > 60000) {
            std::cerr << "Error: Samples should be 1..60000" << std::endl;
            err = 1;
            help = true;
          }
        } else {
          std::cerr << "Error: Specify a sample count as a number" << std::endl;
          err = 1;
          help = true;
        }
        break;
      }
      case 'i': {
        auto iters_arg = ubench::string::parse_int<unsigned int>(optarg);
        if (iters_arg) {
          o.iters_ = *iters_arg;
          if (o.iters_ < 1 || o.iters_ > 60000) {
            std::cerr << "Error: Iterations should be 1..60000" << std::endl;
            err = 1;
            help = true;
          }
        } else {
          std::cerr << "Error: Specify iterations as a number" << std::endl;
          err = 1;
          help = true;
        }
        break;
      }
      case 'b': {
        o.benchmark_name_ = optarg;
        break;
      }
      case '?':
        help = true;
        if (optopt) err = 1;
        break;
      case ':':
        std::cerr << "Error: Option -" << optopt << " requires an operand"
                  << std::endl;
        err = 1;
        help = true;
        break;
      default:
        std::cerr << "Error: Unknown option -" << optopt << std::endl;
        err = 1;
        help = true;
        break;
    }
  }

  if (o.benchmark_name_ == "readwrite") {
    o.core_mode_ = core_mode::mode_readwrite;
  } else {
    auto cm = core_benchmark::mode(o.benchmark_name_);
    o.core_mode_ = core_mode::mode_cas;
    if (!cm) {
      std::cerr << "Error: benchmark type unknown." << std::endl;
      err = 1;
      help = true;
    }
  }

  if (help) {
    if (err) std::cerr << std::endl;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    print_help(std::string_view(argv[0]));
    return stdext::unexpected{err};
  }

  return o;
}
