#include "options.h"

#include <iostream>
#include <string_view>

#include "stdext/expected.h"
#include "ubench/options.h"
#include "ubench/string.h"

namespace {

auto print_help(std::string_view prog_name) -> void {
  std::cout << prog_name << " [-p pid1,pid2,...] [-msr] [-v] [-?]" << std::endl;
  std::cout << std::endl;
  std::cout << " -p list of PIDs to scan" << std::endl;
  std::cout << " -m print shared memory for the PID" << std::endl;
  std::cout << " -s print shared memory with other PIDs" << std::endl;
  std::cout << " -r include read-only shared memory" << std::endl;
  std::cout << " -t print typed memory regions" << std::endl;
  std::cout << " -v increase verbosity" << std::endl;
  std::cout << " -? help" << std::endl;
}

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
auto make_options(int argc, const char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  bool help = false;
  int err = 0;

  options o{};
  ubench::options opts{argc, argv, "cdp:sv?"};
  for (const auto& opt : opts) {
    if (opt) {
      switch (opt->get_option()) {
        case 'v':
          o.verbosity_++;
          break;
        case 'p': {
          o.pids_ =
              // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
              ubench::string::split_args_int<unsigned int>(*opt->argument());
          if (o.pids_.empty()) {
            err = 1;
            std::cerr << "Error: No arguments provided for pid list"
                      << std::endl;
          }
          break;
        }
        case 'm':
          o.phys_mem_ = true;
          break;
        case 's':
          o.shared_mem_ = true;
          break;
        case 'r':
          o.read_ = true;
          break;
        case 't':
          o.tymem_ = true;
          break;
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

    // Even if there was no error, we send an unexpected, but the result will be
    // zero. Indicating no error, but no options either.
    return stdext::unexpected{err};
  }

  // Provide a default if the user didn't provide one.
  if (!o.phys_mem_ && !o.shared_mem_ && !o.tymem_) {
    o.shared_mem_ = true;
  }

  return o;
}
