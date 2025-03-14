#include "options.h"

#include <unistd.h>

#include <iostream>
#include <string>
#include <unordered_map>

#include "stdext/expected.h"

namespace {
void print_help(std::string_view prog_name) {
  std::cout << prog_name << " [-B<impl>] <file>" << std::endl;
  std::cout << std::endl;
  std::cout
      << "Reads the file and interns all individual words for benchmark testing"
      << std::endl;
}

const std::unordered_map<std::string, strintern_impl> mode = {
    {"none", strintern_impl::none},
    {"forward_list", strintern_impl::flist},
    {"set", strintern_impl::set},
    {"unordered_set", strintern_impl::unordered_set},
    {"fixed_set_128k", strintern_impl::fixed_set_128k},
    {"fixed_set_256k", strintern_impl::fixed_set_256k},
    {"fixed_set_512k", strintern_impl::fixed_set_512k},
    {"fixed_set_1m", strintern_impl::fixed_set_1m},
    {"var_set", strintern_impl::var_set},
    {"var_set_pmr", strintern_impl::var_set_pmr},
};

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
    prog_name = std::string_view("str_intern");
  }

  options o{};
  while ((c = getopt(argc, argv, "B:?")) != -1) {
    switch (c) {
      case 'B': {
        auto it = mode.find(optarg);
        if (it != mode.end()) {
          o.mode_ = it->second;
          o.mode_s_ = std::string{optarg};
        } else {
          std::cerr << "Error: Unknown implementation to test: " << optarg
                    << std::endl;
          err = 1;
        }
        break;
      }
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
    std::cerr << "Error: Must provide exactly one single file to read"
              << std::endl;
    err = 1;
  } else {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    o.input_ = argv[optind];
  }

  if (err || help) {
    if (err) std::cerr << std::endl;
    print_help(prog_name);
    return stdext::unexpected{err};
  }

  if (o.mode_s_.empty()) {
    o.mode_s_ = std::string{"none"};
  }

  return o;
}