#include "options.h"

#include <unistd.h>

#include <iostream>
#include <string_view>

#include "ubench/string.h"

namespace {

auto print_short() -> void {
  std::cout << "lsqf [-c] [-d] [-s] [-v] [-p <pid>]" << std::endl;
  std::cout << std::endl;
  std::cout << "List open files on QNX in tabular form." << std::endl;
  std::cout << std::endl;
  std::cout << "-? : Help" << std::endl;
}

auto print_help() -> void {
  print_short();
  std::cout << "-p <pid> : List open files for <pid> (comma separated list), "
               "or all pids"
            << std::endl;
  std::cout << "-c : Show side channels" << std::endl;
  std::cout << "-d : Show dead connections" << std::endl;
  std::cout << "-s : Show connections to self" << std::endl;
  std::cout << "-v : Verbose output" << std::endl;
  std::cout << std::endl;
  std::cout << "Output is in tabular form." << std::endl;
  std::cout << "- Process (PID) : The process queried" << std::endl;
  std::cout << "- FD (Flags) : The FD number and flags as" << std::endl;
  std::cout << "  'c' - close on exec" << std::endl;
  std::cout << "  's' - side channel" << std::endl;
  std::cout << "  'r' - opened for reading" << std::endl;
  std::cout << "  'w' - opened for writing" << std::endl;
  std::cout << "- RsrcMgr (PID) : Resource Manager connected to" << std::endl;
  std::cout << "- Mode : Unix mode for FD opened" << std::endl;
  std::cout << "- Path : Path or Socket of FD" << std::endl;
  std::cout << std::endl;
}

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
auto make_options(int& argc, char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  int c = 0;
  bool help = false;
  int err = 0;
  options o{};

  while ((c = getopt(argc, argv, "cdp:sv?")) != -1) {
    switch (c) {
      case 'c':
        o.show_sidechannels_ = true;
        break;
      case 'd':
        o.show_dead_ = true;
        break;
      case 's':
        o.show_self_ = true;
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
      case 'v':
        o.verbosity_++;
        break;
      case '?':
        if (optopt != '?') {
          err = 1;
        } else {
          help = true;
        }
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

  if (help || err) {
    if (help) {
      print_help();
    } else {
      print_short();
    }
    return stdext::unexpected{err};
  }

  return o;
}