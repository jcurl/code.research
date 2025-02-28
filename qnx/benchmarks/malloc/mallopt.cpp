#include "config.h"

#include "mallopt.h"

#include <malloc.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string_view>

#include <benchmark/benchmark.h>

#include "stdext/expected.h"
#include "ubench/string.h"
#include "mallopt.h"

namespace {

auto print_help(std::string_view prog_name) -> void {
  std::cout << "USAGE: " << prog_name << "[-M<bytes>]";
  if (HAVE_MALLOPT) std::cout << " [-mOPTION=n]";
  if (HAVE_MLOCKALL) std::cout << " [-L]";
  std::cout << std::endl;

  std::cout << "Run a malloc benchmark." << std::endl;
  std::cout << std::endl;
  std::cout << " -M: Specify upper range for test in bytes. Default is "
               "1073741824 (1GB)"
            << std::endl;

  if (HAVE_MLOCKALL) {
    std::cout << " -L: Enable locking with mlockall()." << std::endl;
  }

  if (HAVE_MALLOPT) {
    std::cout << " -mOPTION=value: a comma separated list of mallopt options "
                 "and a value."
              << std::endl;
    std::cout << std::endl;
    impl::print_mallopt_help();
  }

  std::cout << std::endl;
  benchmark::PrintDefaultHelp();
}

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
auto make_options(int& argc, char* const argv[]) noexcept
    -> stdext::expected<mallopt_options, int> {
  bool help = false;
  int c = 0;
  int err = 0;

  std::string options{};
  if (HAVE_MALLOPT) {
    options += "m:";
  }
  if (HAVE_MLOCKALL) {
    options += "L";
  }
  options += "M:?";

  mallopt_options o{};
  while ((c = getopt(argc, argv, options.c_str())) != -1) {
    switch (c) {
      case 'm': {
        auto mallopts = impl::parse_mallopt_arg(optarg);
        if (mallopts.empty()) {
          err = 1;
        }
        o.mallopts_.insert(
            std::end(o.mallopts_), std::begin(mallopts), std::end(mallopts));
        break;
      }
      case 'L':
        o.mlock_all_ = true;
        break;
      case 'M': {
        auto max = ubench::string::parse_int<unsigned int>(optarg);
        if (max) {
          o.max_ = *max;
        } else {
          std::cerr << "Error: Option -" << optopt << " requires an integer"
                    << std::endl;
          err = 1;
          help = true;
        }
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

  if (help) {
    if (err) std::cerr << std::endl;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    print_help(std::string_view(argv[0]));
    return stdext::unexpected{err};
  }

  return o;
}
