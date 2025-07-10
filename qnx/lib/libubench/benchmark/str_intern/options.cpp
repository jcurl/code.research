#include "options.h"

#include <iostream>
#include <string>
#include <unordered_map>

#include "stdext/expected.h"
#include "ubench/options.h"

namespace {
void print_help(std::string_view prog_name) {
  std::cout << prog_name << " [-B<impl>] <file>" << std::endl;
  std::cout << std::endl;
  std::cout
      << "Reads the file and interns all individual words for benchmark testing"
      << std::endl;
}

const std::unordered_map<std::string_view, strintern_impl> mode = {
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
    {"ubench", strintern_impl::ubench},
};

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
auto make_options(int argc, const char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  bool help = false;
  int err = 0;

  options o{};
  ubench::options opts{argc, argv, "B:?"};
  for (const auto& opt : opts) {
    if (opt) {
      switch (opt->get_option()) {
        case 'B': {
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          auto arg = *opt->argument();
          auto it = mode.find(arg);
          if (it != mode.end()) {
            o.mode_ = it->second;
            o.mode_s_ = std::string{arg};
          } else {
            err = 1;
            std::cerr << "Error: Unknown implementation to test: " << arg
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

  if (opts.args().size() != 1) {
    err = 1;
    std::cerr << "Error: Must provide exactly one single file to read"
              << std::endl;
  } else {
    o.input_ = opts.args()[0];
  }

  if (err || help) {
    if (err) std::cerr << std::endl;
    print_help(opts.prog_name());
    return stdext::unexpected{err};
  }

  if (o.mode_s_.empty()) {
    o.mode_s_ = std::string{"none"};
  }

  return o;
}