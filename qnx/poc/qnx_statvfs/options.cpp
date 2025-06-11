#include "options.h"

#include <unistd.h>

#include <iostream>

#include "stdext/expected.h"

namespace {

void print_help(std::string_view prog_name) {
  std::cout << prog_name << " <device>" << std::endl;
  std::cout << std::endl;
  std::cout << " -? - Display this help." << std::endl;
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
    prog_name = std::string_view("qnx_statvfs");
  }

  options o{};
  while ((c = getopt(argc, argv, "?")) != -1) {
    switch (c) {
      case '?':
        if (optopt) err = 1;
        help = true;
        break;
      case ':':
        std::cerr << "Error: Option -" << optopt << " requires an operand"
                  << std::endl;
        err = 1;
        break;
      default:
        std::cerr << "Error: Unknown option -" << optopt << std::endl;
        err = 1;
        break;
    }
  }

  if (argc - optind != 1) {
    // Expect exactly one non-option argument.
    err = 1;
  } else {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    o.device_ = std::string(argv[optind]);
  }

  if (err || help) {
    print_help(prog_name);
    return stdext::unexpected{err};
  }

  return o;
}
