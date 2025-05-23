#include "options.h"

#include <unistd.h>

#include <iostream>
#include <string_view>

#include "stdext/expected.h"
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
auto make_options(int& argc, char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  int c = 0;
  bool help = false;

  std::string_view prog_name{};
  if (argc >= 1) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    prog_name = std::string_view(argv[0]);
  } else {
    prog_name = std::string_view("lshmem");
  }

  int err = 0;
  options o{};
  while ((c = getopt(argc, argv, "p:mrstv?")) != -1) {
    switch (c) {
      case 'v':
        o.verbosity_++;
        break;
      case 'p': {
        o.pids_ = ubench::string::split_args_int<unsigned int>(optarg);
        if (o.pids_.empty()) {
          std::cerr << "Error: No arguments provided for pid list" << std::endl;
          err = 1;
          break;
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

  if (err || help) {
    print_help(prog_name);

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
