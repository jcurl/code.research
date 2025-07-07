#include "ubench/options.h"

#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

#include "gtest/gtest.h"

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof((array)[0]))

namespace {

/// @brief A helper class to manage state while iterating options for tesing.
class options_check {
 public:
  using option_iterator = decltype(std::declval<ubench::options>().begin());
  using arg_iterator =
      decltype(std::declval<ubench::options>().args().cbegin());

  options_check(ubench::options options) : options_{std::move(options)} {
    option_ = options_.begin();
    arg_ = options_.args().cbegin();
  }

  auto has_error(char option, ubench::option_error_kind kind)
      -> testing::AssertionResult {
    if (option_ == options_.end()) {
      return testing::AssertionFailure() << "no more options";
    }

    if (option_->has_value()) {
      auto v = option_->value();
      option_++;
      return testing::AssertionFailure()
             << "arg " << v.get_option() << " has a value";
    }

    auto ve = option_->error();

    if (ve.get_option() != option) {
      option_++;
      return testing::AssertionFailure()
             << "option error for " << ve.get_option() << ", expected "
             << option;
    }

    if (ve.kind() != kind) {
      option_++;
      return testing::AssertionFailure()
             << "arg " << ve.get_option() << " has error kind "
             << error_kind(ve.kind()) << ", expected " << error_kind(kind);
    }

    option_++;
    return testing::AssertionSuccess();
  };

  auto has_option(char option) -> testing::AssertionResult {
    if (option_ == options_.end()) {
      return testing::AssertionFailure() << "no more options";
    }

    if (!option_->has_value()) {
      auto ve = option_->error();
      option_++;
      return testing::AssertionFailure()
             << "option error for " << ve.get_option()
             << ", expected no error for " << option;
    }

    const auto v = option_->value();
    if (v.get_option() != option) {
      option_++;
      return testing::AssertionFailure()
             << "got option " << v.get_option() << ", expected " << option;
    }

    const auto va = v.argument();
    if (va.has_value()) {
      option_++;
      return testing::AssertionFailure()
             << "option " << option << "got argument " << va.value()
             << ", expected to have no arguments";
    }

    option_++;
    return testing::AssertionSuccess();
  }

  auto has_option(char option, const std::string& arg)
      -> testing::AssertionResult {
    if (option_ == options_.end()) {
      return testing::AssertionFailure() << "no more options";
    }

    if (!option_->has_value()) {
      auto ve = option_->error();
      option_++;
      return testing::AssertionFailure() << "got error for " << ve.get_option()
                                         << ", expected optiom " << option;
    }

    const auto v = option_->value();
    if (v.get_option() != option) {
      option_++;
      return testing::AssertionFailure() << "got option " << v.get_option()
                                         << ", expected option " << option;
    }

    const auto va = v.argument();
    if (!va.has_value()) {
      option_++;
      return testing::AssertionFailure()
             << "option " << option << " got no argument, expected " << arg;
    }

    if (va.value() != arg) {
      option_++;
      return testing::AssertionFailure()
             << "option " << option << " got argument " << va.value()
             << ", expected argument " << arg;
    }

    option_++;
    return testing::AssertionSuccess();
  }

  auto is_end() -> testing::AssertionResult {
    if (option_ != options_.end())
      return testing::AssertionFailure() << "not final option";
    return testing::AssertionSuccess();
  }

  auto has_arg(const std::string& arg) -> testing::AssertionResult {
    if (arg_ == options_.args().cend())
      return testing::AssertionFailure() << "no more arguments";

    if (arg != *arg_) {
      auto a = *arg_;
      arg_++;
      return testing::AssertionFailure()
             << "got argument " << a << ", expected " << arg;
    }

    arg_++;
    return testing::AssertionSuccess();
  }

  auto last_arg() -> testing::AssertionResult {
    if (arg_ != options_.args().cend())
      return testing::AssertionFailure() << "more arguments available";
    return testing::AssertionSuccess();
  }

 private:
  ubench::options options_;
  option_iterator option_;
  arg_iterator arg_;

  auto error_kind(ubench::option_error_kind kind) -> std::string {
    switch (kind) {
      case ubench::option_error_kind::missing:
        return "missing";
      case ubench::option_error_kind::unexpected:
        return "unexpected";
      default:
        return "unknown";
    }
  }
};

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define OPTIONS_TEST(function, argv, optstring)                               \
  function(ubench::options(ARRAY_LENGTH(argv), argv, optstring));             \
  function(ubench::options((argv)[0],                                         \
      std::vector<std::string>(&(argv)[1], &(argv)[ARRAY_LENGTH(argv)]),      \
      optstring));                                                            \
  function(ubench::options((argv)[0],                                         \
      std::vector<std::string_view>(&(argv)[1], &(argv)[ARRAY_LENGTH(argv)]), \
      optstring));

auto options_noopts(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_error('f', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
};

TEST(options, noopts) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_noopts, argv, "");
}

auto options_oneopt(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f'));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, oneopt) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_oneopt, argv, "f");
}

auto options_oneopt_arg1(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f'));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("arg"));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, oneopt_arg1) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "arg"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_oneopt_arg1, argv, "f");
}

auto options_oneopt_arg2(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f'));
  ASSERT_TRUE(optcheck.has_error('a', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_error('r', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_error('g', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, oneopt_arg2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-farg"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_oneopt_arg2, argv, "f");
}

auto options_oneoptarg_none(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_error('f', ubench::option_error_kind::missing));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, oneoptarg_none) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_oneoptarg_none, argv, "f:");
}

auto options_oneoptarg_none2(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('a'));
  ASSERT_TRUE(optcheck.has_error('f', ubench::option_error_kind::missing));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, oneoptarg_none2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-af"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_oneoptarg_none2, argv, "af:");
}

auto options_oneoptarg_arg(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f', "arg"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, oneoptarg_arg) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-farg"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_oneoptarg_arg, argv, "f:");
}

auto options_oneoptarg_arg2(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f', "arg"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, oneoptarg_arg2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "arg"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_oneoptarg_arg2, argv, "f:");
}

auto options_oneoptarg_arg3(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f', "a"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, oneoptarg_arg3) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "a"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_oneoptarg_arg3, argv, "f:a");
}

auto options_oneoptarg_arg4(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f', "a"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, oneoptarg_arg4) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-fa"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_oneoptarg_arg4, argv, "f:a");
}

auto options_oneoptarg_equals(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f', "=arg"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, oneoptarg_equals) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f=arg"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_oneoptarg_equals, argv, "f:");
}

auto options_twooptsmix(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f', "arg"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, twooptsmix) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-farg"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_twooptsmix, argv, "f:a");
}

auto options_twooptsmix2(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('a'));
  ASSERT_TRUE(optcheck.has_option('f', "arg"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, twooptsmix2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-afarg"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_twooptsmix2, argv, "f:a");
}

auto options_twooptsmix3(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('a'));
  ASSERT_TRUE(optcheck.has_option('f', "arg"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, twooptsmix3) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-a", "-farg"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_twooptsmix3, argv, "f:a");
}

auto options_twooptsmix4(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('a'));
  ASSERT_TRUE(optcheck.has_option('f', "-farg"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, twooptsmix4) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-af", "-farg"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_twooptsmix4, argv, "f:a");
}

auto options_argfirst_noopts(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("arg"));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, argfirst_noopts) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "arg"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_argfirst_noopts, argv, "f:a");
}

auto options_argfirst_oneopt(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("arg"));
  ASSERT_TRUE(optcheck.has_arg("-f"));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, argfirst_oneopt) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "arg", "-f"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_argfirst_oneopt, argv, "f");
}

auto options_argfirst2_oneopt(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("arg"));
  ASSERT_TRUE(optcheck.has_arg("-f"));
  ASSERT_TRUE(optcheck.has_arg("arg2"));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, argfirst2_oneopt) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "arg", "-f", "arg2"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_argfirst2_oneopt, argv, "f");
}

auto options_invalidopt(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_error('z', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, invalidopt) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-z"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_invalidopt, argv, "f");
}

auto options_invalidopt_arg(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_error('z', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("arg"));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, invalidopt_arg) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-z", "arg"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_invalidopt_arg, argv, "f");
}

auto options_invalidopt_char1(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_error(':', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("arg"));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, invalidopt_char1) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-:", "arg"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_invalidopt_char1, argv, "f");
}

TEST(options, invalidopt_char1b) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-:", "arg"};

  ASSERT_THROW(
      {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        ubench::options(ARRAY_LENGTH(argv), argv, ":f");
      },
      std::invalid_argument);

  ASSERT_THROW(
      {
        ubench::options("prog_name",
            std::vector<std::string>(&argv[1], &argv[ARRAY_LENGTH(argv)]),
            ":f");
      },
      std::invalid_argument);

  ASSERT_THROW(
      {
        ubench::options("prog_name",
            std::vector<std::string_view>(&argv[1], &argv[ARRAY_LENGTH(argv)]),
            ":f");
      },
      std::invalid_argument);
}

TEST(options, invalidopt_char2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-<", "arg"};

  ASSERT_THROW(
      {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        options_check(ubench::options(ARRAY_LENGTH(argv), argv, "f:<"));
      },
      std::invalid_argument);

  ASSERT_THROW(
      {
        ubench::options("prog_name",
            std::vector<std::string>(&argv[1], &argv[ARRAY_LENGTH(argv)]),
            "f:<");
      },
      std::invalid_argument);

  ASSERT_THROW(
      {
        ubench::options("prog_name",
            std::vector<std::string_view>(&argv[1], &argv[ARRAY_LENGTH(argv)]),
            "f:<");
      },
      std::invalid_argument);
}

TEST(options, invalidopt_char3) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-@", "arg"};

  ASSERT_THROW(
      {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        options_check(ubench::options(ARRAY_LENGTH(argv), argv, "f:@"));
      },
      std::invalid_argument);

  ASSERT_THROW(
      {
        ubench::options("prog_name",
            std::vector<std::string>(&argv[1], &argv[ARRAY_LENGTH(argv)]),
            "f:@");
      },
      std::invalid_argument);

  ASSERT_THROW(
      {
        ubench::options("prog_name",
            std::vector<std::string_view>(&argv[1], &argv[ARRAY_LENGTH(argv)]),
            "f:@");
      },
      std::invalid_argument);
}

TEST(options, invalidopt_char4) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-;", "arg"};

  ASSERT_THROW(
      {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        options_check(ubench::options(ARRAY_LENGTH(argv), argv, "f:;"));
      },
      std::invalid_argument);

  ASSERT_THROW(
      {
        ubench::options("prog_name",
            std::vector<std::string>(&argv[1], &argv[ARRAY_LENGTH(argv)]),
            "f:;");
      },
      std::invalid_argument);

  ASSERT_THROW(
      {
        ubench::options("prog_name",
            std::vector<std::string_view>(&argv[1], &argv[ARRAY_LENGTH(argv)]),
            "f:;");
      },
      std::invalid_argument);
}

auto options_help(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('?'));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, help) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-?"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_help, argv, "abcf?");
}

auto options_verbose(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('v'));
  ASSERT_TRUE(optcheck.has_option('v'));
  ASSERT_TRUE(optcheck.has_option('v'));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, verbose) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-vvv"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_verbose, argv, "abcfv?");
}

auto options_missing_arg_twoargs(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f', "-a"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, missing_arg_twoargs) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "-a"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_missing_arg_twoargs, argv, "f:a");
}

auto options_argwhitespace(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f', " "));
  ASSERT_TRUE(optcheck.has_option('a'));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, argwhitespace) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f ", "-a"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_argwhitespace, argv, "f:a");
}

auto options_minus1(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("-"));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, minus1) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_minus1, argv, "f:a");
}

auto options_minus2(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('a'));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("-"));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, minus2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-a", "-"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_minus2, argv, "f:a");
}

auto options_minus3(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("-"));
  ASSERT_TRUE(optcheck.has_arg("-a"));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, minus3) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-", "-a"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_minus3, argv, "f:a");
}

auto options_minus4(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f', "-"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, minus4) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "-"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_minus4, argv, "f:a");
}

auto options_minus4b(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f', "-"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, minus4b) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f-"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_minus4b, argv, "f:a");
}

auto options_doubleminus1(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, doubleminus1) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "--"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_doubleminus1, argv, "f:a");
}

auto options_doubleminus2(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('a'));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, doubleminus2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-a", "--"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_doubleminus2, argv, "f:a");
}

auto options_doubleminus3(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("-a"));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, doubleminus3) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "--", "-a"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_doubleminus3, argv, "f:a");
}

auto options_doubleminus4(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f', "--"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, doubleminus4) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "--"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_doubleminus4, argv, "f:a");
}

auto options_doubleminus4b(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f', "--"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, doubleminus4b) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f--"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_doubleminus4b, argv, "f:a");
}

auto options_longoption1(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_error('-', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_error('l', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_error('o', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_error('n', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_error('g', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, longoption1) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "--long"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_longoption1, argv, "f:a");
}

auto options_longoption2(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f', "--long"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, longoption2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "--long"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_longoption2, argv, "f:a");
}

auto options_longoption3(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('a'));
  ASSERT_TRUE(optcheck.has_error('-', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_error('l', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_error('o', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_error('n', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_error('g', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, longoption3) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-a", "--long"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_longoption3, argv, "f:a");
}

auto options_longoption4(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_error('-', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_error('l', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_error('o', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_error('n', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_error('g', ubench::option_error_kind::unexpected));
  ASSERT_TRUE(optcheck.has_option('a'));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, longoption4) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "--long", "-a"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_longoption4, argv, "f:a");
}

auto options_vendoropt1(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('a'));
  ASSERT_TRUE(optcheck.has_option('W'));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("X"));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, vendoropt1) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-a", "-W", "X"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_vendoropt1, argv, "f:aW");
}

auto options_vendoropt2(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('W'));
  ASSERT_TRUE(optcheck.has_option('a'));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("X"));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, vendoropt2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-W", "-a", "X"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_vendoropt2, argv, "f:aW");
}

auto options_vendoropt3(ubench::options options) -> void {
  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('a'));
  ASSERT_TRUE(optcheck.has_option('W', "X"));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, vendoropt3) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-a", "-W", "X"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_vendoropt3, argv, "f:aW:");
}

auto options_prog_name(ubench::options options) -> void {
  ASSERT_EQ(options.prog_name(), "prog_name");
}

TEST(options, prog_name) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-a", "-W", "X"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_prog_name, argv, "f:aW:");
}

auto options_move_operator(ubench::options options) -> void {
  auto optcheck = options_check(options);
  ASSERT_TRUE(optcheck.has_option('o', "file"));
  ASSERT_TRUE(optcheck.has_option('v'));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("input"));
  ASSERT_TRUE(optcheck.last_arg());

  auto newopts = std::move(options);
  auto optcheck2 = options_check(newopts);
  ASSERT_TRUE(optcheck2.has_option('o', "file"));
  ASSERT_TRUE(optcheck2.has_option('v'));
  ASSERT_TRUE(optcheck2.is_end());
  ASSERT_TRUE(optcheck2.has_arg("input"));
  ASSERT_TRUE(optcheck2.last_arg());
}

TEST(options, move_operator) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-o", "file", "-v", "input"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_move_operator, argv, "o:v");
}

auto options_copy_operator(ubench::options options) -> void {
  auto optcheck = options_check(options);
  ASSERT_TRUE(optcheck.has_option('o', "file"));
  ASSERT_TRUE(optcheck.has_option('v'));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("input"));
  ASSERT_TRUE(optcheck.last_arg());

  auto newopts = options;
  auto optcheck2 = options_check(newopts);
  ASSERT_TRUE(optcheck2.has_option('o', "file"));
  ASSERT_TRUE(optcheck2.has_option('v'));
  ASSERT_TRUE(optcheck2.is_end());
  ASSERT_TRUE(optcheck2.has_arg("input"));
  ASSERT_TRUE(optcheck2.last_arg());
}

TEST(options, copy_operator) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-o", "file", "-v", "input"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  OPTIONS_TEST(options_copy_operator, argv, "o:v");
}

auto options_null_progname(ubench::options options) -> void {
  ASSERT_EQ(options.prog_name(), "");

  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('v'));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("input"));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, null_progname) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {nullptr, "-v", "input"};

  // Not all three constructors are tested, as it's undefined behaviour to
  // create a string, or a string_view from a nullptr.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  options_null_progname(ubench::options(ARRAY_LENGTH(argv), argv, "v"));
}

auto options_null_arg(ubench::options options) -> void {
  ASSERT_EQ(options.prog_name(), "prog_name");

  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('v'));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg(""));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, null_arg) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-v", nullptr};

  // Not all three constructors are tested, as it's undefined behaviour to
  // create a string, or a string_view from a nullptr.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  options_null_arg(ubench::options(ARRAY_LENGTH(argv), argv, "v"));
}

auto options_null_opt(ubench::options options) -> void {
  ASSERT_EQ(options.prog_name(), "prog_name");

  auto optcheck = options_check(std::move(options));
  ASSERT_TRUE(optcheck.has_option('f', ""));
  ASSERT_TRUE(optcheck.is_end());
  ASSERT_TRUE(optcheck.has_arg("input"));
  ASSERT_TRUE(optcheck.last_arg());
}

TEST(options, null_opt) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", nullptr, "input"};

  // Not all three constructors are tested, as it's undefined behaviour to
  // create a string, or a string_view from a nullptr.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  options_null_opt(ubench::options(ARRAY_LENGTH(argv), argv, "f:"));
}
