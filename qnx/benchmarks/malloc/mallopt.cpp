#include "config.h"

#include "mallopt.h"

#include <malloc.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string_view>

#include <benchmark/benchmark.h>

#include "stdext/expected.h"
#include "ubench/options.h"
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
auto make_options(int argc, const char* const argv[]) noexcept
    -> stdext::expected<mallopt_options, int> {
  bool help = false;
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
  ubench::options opts{argc, argv, options.c_str()};
  for (const auto& opt : opts) {
    if (opt) {
      switch (opt->get_option()) {
        case 'm': {
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          auto mallopts = impl::parse_mallopt_arg(*opt->argument());
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
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          auto max = ubench::string::parse_int<unsigned int>(*opt->argument());
          if (max) {
            o.max_ = *max;
          } else {
            err = 1;
            std::cerr << "Error: Option -" << opt->get_option()
                      << " requires an integer" << std::endl;
          }
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

  if (err | help) {
    if (err) std::cerr << std::endl;
    print_help(opts.prog_name());
    return stdext::unexpected{err};
  }

  return o;
}
