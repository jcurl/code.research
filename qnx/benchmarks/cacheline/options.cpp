#include "options.h"

#include <unistd.h>

#include <iostream>
#include <string_view>

#include <benchmark/benchmark.h>

#include "stdext/expected.h"
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
auto make_options(int& argc, char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  int c = 0;
  bool help = false;
  int err = 0;

  std::string_view prog_name{};
  if (argc >= 1) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    prog_name = std::string_view(argv[0]);
  } else {
    prog_name = std::string_view("cacheline_bench");
  }

  options o{};
  while ((c = getopt(argc, argv, "b:?")) != -1) {
    switch (c) {
      case 'b': {
        auto buffer_arg = ubench::string::parse_int<std::uint32_t>(optarg);
        if (buffer_arg) {
          o.buffer_size_ = *buffer_arg;
          if (o.buffer_size_ < 1 || o.buffer_size_ > 512) {
            std::cerr
                << "Error: Buffer size should be from 1 to 512 (units of MB)"
                << std::endl;
            err = 1;
            help = true;
          }
        } else {
          std::cerr << "Error: Specify a buffer sizse as a number" << std::endl;
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