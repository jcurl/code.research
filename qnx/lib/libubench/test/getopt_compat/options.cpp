#include "config.h"

#include "getopt_compat/options.h"

#if HAVE_FEATURES_H
#include <features.h>
#endif
#include <unistd.h>

namespace ubench::experimental {

namespace {

// An empty string that can be returned as a reference.
const std::string empty{};

auto reset_getopt() -> void {
#ifdef __GNU_LIBRARY__
  // On GNU systems, must be to zero, that it internally resets the parsing
  // logic. Setting to the default of 1 doesn't reset the parsing state that a
  // second vector can be parsed, if getopt() was called prior.
  ::optind = 0;
#else
  ::optind = 1;
#endif
  ::optopt = 0;
}

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
options::options(int argc, const char* const argv[], const char* options)
    : options_{options} {
  if (argc == 0) return;
  if (argv == nullptr) return;

  if (argc > 1) {
    for (int i = 1; i < argc; i++) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      if (argv[i] == nullptr) {
        args_.emplace_back();
      } else {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        args_.emplace_back(argv[i]);
      }
    }
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  prog_name_ = std::string{argv[0]};
  reset_getopt();
}

options::options(
    std::string prog_name, std::vector<std::string> args, std::string options)
    : prog_name_{std::move(prog_name)},
      args_{std::move(args)},
      options_{std::move(options)} {
  reset_getopt();
};

auto options::reset() -> void { reset_getopt(); }

auto options::getopt() -> int {
  if (getopt_args_.empty()) {
    if (args_.empty()) return -1;

    // Need to copy from our C++ implementation to something that the C-library
    // likes.
    //
    // WARNING: the vector that is now being copied may not change! If
    // so, then the refs in getopt_args_ will be invalidated resulting in
    // undefined behaviour.

    getopt_args_.push_back(prog_name_.data());
    for (auto& arg : args_) {
      getopt_args_.push_back(arg.data());
    }
  }

  // On non-GNU, this variable might not be reset to 'nullptr' if the argument
  // has no options.
  ::optarg = nullptr;
  int opt = ::getopt(static_cast<int>(getopt_args_.size()), getopt_args_.data(),
      options_.c_str());
  return opt;
}

auto options::optarg() -> std::optional<std::string> {
  if (::optarg) return std::string{::optarg};
  return {};
}

auto options::optopt() -> int { return ::optopt; }

auto options::prog_name() -> const std::string& { return prog_name_; }

}  // namespace ubench::experimental
