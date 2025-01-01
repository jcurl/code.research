#include "config.h"

#include "mallopt.h"

#include <unistd.h>

#include <cerrno>
#include <charconv>
#include <cstring>
#include <iostream>
#include <map>
#include <optional>
#include <string_view>

#include <benchmark/benchmark.h>

#include "mlock.h"

mallopt_options::mallopt_options(int argc, char **argv) {
  bool help = false;
  int c = 0;

  std::string options{};
  if (HAVE_MALLOPT) {
    options += "m:";
  }
  if (HAVE_MLOCKALL) {
    options += "L";
  }
  options += "?";

  while ((c = getopt(argc, argv, options.c_str())) != -1) {
    switch (c) {
      case 'm': {
        if (!parse_mallopt_arg(std::string_view(optarg))) {
          this->result_ = 1;
        }
        break;
      }
      case 'L': {
        auto error = enable_mlockall();
        if (error) {
          std::cout << "Error locking with -L, " << error.value() << std::endl;
          this->result_ = 1;
          return;
        } else {
          std::cout << "Memory Locking ENABLED" << std::endl;
        }
        break;
      }
      case '?':
        help = true;
        if (optopt) this->result_ = 1;
        break;
      case ':':
        std::cerr << "Error: Option -" << optopt << " requires an operand"
                  << std::endl;
        this->result_ = 1;
        help = true;
        break;
      default:
        std::cerr << "Error: Unknown option -" << optopt << std::endl;
        this->result_ = 1;
        help = true;
        break;
    }
  }

  if (help) {
    if (this->result_) std::cerr << std::endl;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    print_help(std::string_view(argv[0]));
    this->do_run_ = false;
  }

  if (this->result_) this->do_run_ = false;
}

auto mallopt_options::result() -> int { return this->result_; }

auto mallopt_options::do_run() -> bool { return this->do_run_; }

auto mallopt_options::print_help(std::string_view prog_name) -> void {
  std::cout << "USAGE: " << prog_name;
  if (HAVE_MALLOPT) std::cout << " [-mOPTION=n]";
  if (HAVE_MLOCKALL) std::cout << " [-L]";
  std::cout << std::endl;

  std::cout << std::endl;

  std::cout << "Run a malloc benchmark." << std::endl;
  std::cout << std::endl;

  if (HAVE_MLOCKALL) {
    std::cout << " -L: Enable locking with mlockall()." << std::endl;
  }

  if (HAVE_MALLOPT) {
    std::cout << " -mOPTION=value: a comma separated list of mallopt options "
                 "and a value."
              << std::endl;
    std::cout << std::endl;
    print_mallopt_help();
  }

  std::cout << std::endl;
  benchmark::PrintDefaultHelp();
}
