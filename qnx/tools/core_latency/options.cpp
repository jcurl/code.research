#include "options.h"

#include <iostream>

#include "stdext/expected.h"
#include "ubench/options.h"
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
auto make_options(int argc, const char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  bool help = false;
  int err = 0;

  options o{};
  ubench::options opts{argc, argv, "s:i:b:?"};
  for (const auto& opt : opts) {
    if (opt) {
      switch (opt->get_option()) {
        case 's': {
          auto samples_arg =
              // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
              ubench::string::parse_int<unsigned int>(*opt->argument());
          if (samples_arg) {
            o.samples_ = *samples_arg;
            if (o.samples_ < 1 || o.samples_ > 60000) {
              err = 1;
              std::cerr << "Error: Samples should be 1..60000" << std::endl;
            }
          } else {
            err = 1;
            std::cerr << "Error: Specify a sample count as a number"
                      << std::endl;
          }
          break;
        }
        case 'i': {
          auto iters_arg =
              // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
              ubench::string::parse_int<unsigned int>(*opt->argument());
          if (iters_arg) {
            o.iters_ = *iters_arg;
            if (o.iters_ < 1 || o.iters_ > 60000) {
              err = 1;
              std::cerr << "Error: Iterations should be 1..60000" << std::endl;
            }
          } else {
            err = 1;
            std::cerr << "Error: Specify iterations as a number" << std::endl;
          }
          break;
        }
        case 'b': {
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          o.benchmark_name_ = *opt->argument();
          break;
        }
        case '?':
          help = true;
          break;
        default:
          err = 1;
          ubench::options::print_error(opt->get_option());
          break;
      }
    } else {
      err = 1;
      ubench::options::print_error(opt.error());
    }
  }

  if (o.benchmark_name_ == "readwrite") {
    o.core_mode_ = core_mode::mode_readwrite;
  } else {
    auto cm = core_benchmark::mode(o.benchmark_name_);
    o.core_mode_ = core_mode::mode_cas;
    if (!cm) {
      err = 1;
      std::cerr << "Error: benchmark type unknown." << std::endl;
    }
  }

  if (err | help) {
    if (err) std::cerr << std::endl;
    print_help(opts.prog_name());
    return stdext::unexpected{err};
  }

  return o;
}
