#include "options.h"

#include <iostream>
#include <string_view>

#include "ubench/options.h"
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
auto make_options(int argc, const char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  bool help = false;
  int err = 0;

  options o{};
  ubench::options opts{argc, argv, "cdp:sv?"};
  for (const auto& opt : opts) {
    if (opt) {
      switch (opt->get_option()) {
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
        case 'v':
          o.verbosity_++;
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