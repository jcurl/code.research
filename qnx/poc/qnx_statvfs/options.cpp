#include "options.h"

#include <iostream>

#include "stdext/expected.h"
#include "ubench/options.h"

namespace {

void print_help(std::string_view prog_name) {
  std::cout << prog_name << " <device>" << std::endl;
  std::cout << std::endl;
  std::cout << " -? - Display this help." << std::endl;
}

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
auto make_options(int argc, const char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  bool help = false;
  int err = 0;

  options o{};
  ubench::options opts{argc, argv, "?"};
  for (const auto& opt : opts) {
    if (opt) {
      switch (opt->get_option()) {
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

  if (opts.args().size() != 1) {
    // Expect exactly one non-option argument.
    err = 1;
  } else {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    o.device_ = opts.args()[0];
  }

  if (err || help) {
    if (err) std::cerr << std::endl;
    print_help(opts.prog_name());
    return stdext::unexpected{err};
  }

  return o;
}
