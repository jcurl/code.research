#include "options.h"

#include <iostream>
#include <string_view>

#include <benchmark/benchmark.h>

#include "stdext/expected.h"
#include "ubench/options.h"
#include "ubench/string.h"

namespace {

auto print_help(std::string_view prog_name) -> void {
  std::cout << "USAGE: " << prog_name << " [-b <buffer>]" << std::endl;
  std::cout << std::endl;
  std::cout << "Execute strided copy test for <buffer> MB (default is 256MB)."
            << std::endl;

  std::cout << std::endl;
  benchmark::PrintDefaultHelp();
}

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
auto make_options(int argc, const char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  bool help = false;
  int err = 0;

  options o{};
  ubench::options opts{argc, argv, "b:?"};
  for (const auto& opt : opts) {
    if (opt) {
      switch (opt->get_option()) {
        case 'b': {
          auto buffer_arg =
              // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
              ubench::string::parse_int<std::uint32_t>(*opt->argument());
          if (buffer_arg) {
            o.buffer_size_ = *buffer_arg;
            if (o.buffer_size_ < 1 || o.buffer_size_ > 512) {
              err = 1;
              std::cerr
                  << "Error: Buffer size should be from 1 to 512 (units of MB)"
                  << std::endl;
            }
          } else {
            err = 1;
            std::cerr << "Error: Specify a buffer size as a number"
                      << std::endl;
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

  if (err || help) {
    if (err) std::cerr << std::endl;
    print_help(opts.prog_name());
    return stdext::unexpected{err};
  }

  return o;
}
