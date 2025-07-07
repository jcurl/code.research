#include "config.h"

#include <gtest/gtest.h>

#include "getopt_compat/options.h"

#if HAVE_FEATURES_H
#include <features.h>

#ifdef __GNU_LIBRARY__
#define TEST_GNU_EXTENSION
#endif
#endif

namespace {

auto has_option(const std::optional<std::string>& arg) -> bool {
  return arg.has_value();
}

auto has_option(const std::optional<std::string>& arg, std::string value)
    -> bool {
  return arg.has_value() && arg.value() == value;
}

}  // namespace

TEST(ubench_options, noargs_main) {
  auto options = ubench::experimental::options{0, nullptr, ""};
  ASSERT_EQ(options.prog_name(), std::string{});
}

TEST(ubench_options, onearg_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{1, argv, ""};
  ASSERT_EQ(options.prog_name(), "prog_name");
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, twoargs_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "arg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{2, argv, ""};
  ASSERT_EQ(options.prog_name(), "prog_name");
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, noargs) {
  auto options = ubench::experimental::options{"", {}, ""};
  ASSERT_EQ(options.prog_name(), std::string{});
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, onearg) {
  auto options = ubench::experimental::options{"prog_name", {}, ""};
  ASSERT_EQ(options.prog_name(), "prog_name");
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, twoargs) {
  auto options = ubench::experimental::options{"prog_name", {"arg"}, ""};
  ASSERT_EQ(options.prog_name(), "prog_name");
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, parseargs_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{2, argv, "f"};
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_FALSE(has_option(options.optarg()));
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, parseargs) {
  auto options = ubench::experimental::options{"prog_name", {"-f"}, "f"};
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_FALSE(has_option(options.optarg()));
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, parseargs_req1_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "arg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{3, argv, "f:"};
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, parseargs_req2_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {
      "prog_name",
      "-farg",
  };

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{2, argv, "f:"};
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, parseargs_req1) {
  auto options =
      ubench::experimental::options{"prog_name", {"-f", "arg"}, "f:"};
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, parseargs_req2) {
  auto options = ubench::experimental::options{"prog_name", {"-farg"}, "f:"};
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), -1);
}

#ifdef TEST_GNU_EXTENSION
TEST(ubench_options, parseargs_optexist1_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "arg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{3, argv, "f::"};
  ASSERT_EQ(options.getopt(), 'f');

  // GNU extension. Two colons mean an option takes an optional arg; if there is
  // text in the  current argv-element  (i.e.,  in  the  same word as the option
  // name itself, for example, "-oarg"), then it is returned in optarg,
  // otherwise optarg is set to zero.
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, parseargs_optexist2_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-farg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{2, argv, "f::"};
  ASSERT_EQ(options.getopt(), 'f');

  // GNU extension. Two colons mean an option takes an optional arg; if there is
  // text in the  current argv-element  (i.e.,  in  the  same word as the option
  // name itself, for example, "-oarg"), then it is returned in optarg,
  // otherwise optarg is set to zero.
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, parseargs_optmissing_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{3, argv, "f::"};
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_FALSE(has_option(options.optarg()));
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, parseargs_optexist1) {
  auto options =
      ubench::experimental::options{"prog_name", {"-f", "arg"}, "f::"};
  ASSERT_EQ(options.getopt(), 'f');

  // GNU extension. Two colons mean an option takes an optional arg; if there is
  // text in the  current argv-element  (i.e.,  in  the  same word as the option
  // name itself, for example, "-oarg"), then it is returned in optarg,
  // otherwise optarg is set to zero.
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, parseargs_optexist2) {
  auto options = ubench::experimental::options{"prog_name", {"-farg"}, "f::"};
  ASSERT_EQ(options.getopt(), 'f');

  // GNU extension. Two colons mean an option takes an optional arg; if there is
  // text in the  current argv-element  (i.e.,  in  the  same word as the option
  // name itself, for example, "-oarg"), then it is returned in optarg,
  // otherwise optarg is set to zero.
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, parseargs_optmissing) {
  auto options = ubench::experimental::options{"prog_name", {"-f"}, "f::"};
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_FALSE(has_option(options.optarg()));
  ASSERT_EQ(options.getopt(), -1);
}
#endif

TEST(ubench_options, parseargs_mix1_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "arg", "-g"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{4, argv, "f:g"};
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), 'g');
  ASSERT_FALSE(has_option(options.optarg()));
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, parseargs_mix2_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-g", "-f", "arg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{4, argv, "f:g"};
  ASSERT_EQ(options.getopt(), 'g');
  ASSERT_FALSE(has_option(options.optarg()));
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, parseargs_mix1) {
  auto options =
      ubench::experimental::options("prog_name", {"-f", "arg", "-g"}, "f:g");
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), 'g');
  ASSERT_FALSE(has_option(options.optarg()));
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, parseargs_mix2) {
  auto options =
      ubench::experimental::options("prog_name", {"-g", "-f", "arg"}, "f:g");
  ASSERT_EQ(options.getopt(), 'g');
  ASSERT_FALSE(options.optarg().has_value());
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), -1);
}

TEST(ubench_options, reset_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "arg", "-g"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{4, argv, "f:g"};
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), 'g');
  ASSERT_FALSE(has_option(options.optarg()));

  options.reset();
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), 'g');
  ASSERT_FALSE(has_option(options.optarg()));
}

TEST(ubench_options, reset) {
  auto options =
      ubench::experimental::options{"prog_name", {"-f", "arg", "-g"}, "f:g"};
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), 'g');
  ASSERT_FALSE(has_option(options.optarg()));

  options.reset();
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), 'g');
  ASSERT_FALSE(has_option(options.optarg()));
}

TEST(ubench_options, reset_middle_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "arg", "-g"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{4, argv, "f:g"};
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));

  options.reset();
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), 'g');
  ASSERT_FALSE(has_option(options.optarg()));
}

TEST(ubench_options, reset_middle) {
  auto options =
      ubench::experimental::options{"prog_name", {"-f", "arg", "-g"}, "f:g"};
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));

  options.reset();
  ASSERT_EQ(options.getopt(), 'f');
  ASSERT_TRUE(has_option(options.optarg(), "arg"));
  ASSERT_EQ(options.getopt(), 'g');
  ASSERT_FALSE(has_option(options.optarg()));
}

TEST(ubench_options, unknown_option_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-x"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{2, argv, "f:g"};
  ASSERT_EQ(options.getopt(), '?');
  ASSERT_EQ(options.optopt(), 'x');
  ASSERT_FALSE(has_option(options.optarg()));
}

TEST(ubench_options, unknown_option) {
  auto options = ubench::experimental::options{"prog_name", {"-x"}, "f:g"};
  ASSERT_EQ(options.getopt(), '?');
  ASSERT_EQ(options.optopt(), 'x');
  ASSERT_FALSE(has_option(options.optarg()));
}

TEST(ubench_options, help_option_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-x", "-?"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{3, argv, "x?"};
  ASSERT_EQ(options.getopt(), 'x');
  EXPECT_EQ(options.optopt(), 0);  // Returns 'x' from previous test case
  ASSERT_EQ(options.getopt(), '?');
  EXPECT_EQ(options.optopt(), 0);  // Returns 'x' from previous test case
}

TEST(ubench_options, help_option2_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-a", "-x", "-?"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{4, argv, "x?"};
  ASSERT_EQ(options.getopt(), '?');
  EXPECT_EQ(options.optopt(), 'a');
  ASSERT_EQ(options.getopt(), 'x');
  EXPECT_EQ(options.optopt(), 0);  // Returns 'a' from previous option
  ASSERT_EQ(options.getopt(), '?');
  EXPECT_EQ(options.optopt(), 0);  // Returns 'a' from previous option
}

TEST(ubench_options, missing_arg1_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-x"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{2, argv, "x:v"};
  ASSERT_EQ(options.getopt(), '?');
  ASSERT_EQ(options.optopt(), 'x');
}

TEST(ubench_options, missing_arg2_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-x"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{2, argv, ":x:v"};
  ASSERT_EQ(options.getopt(), ':');
  ASSERT_EQ(options.optopt(), 'x');
}

TEST(ubench_options, missing_arg3_main) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-x", "-v"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options{3, argv, ":x:v"};
  ASSERT_EQ(options.getopt(), ':');
  ASSERT_EQ(options.optopt(), 'x');
  ASSERT_FALSE(has_option(options.optarg()));
  ASSERT_EQ(options.getopt(), 'v');
  EXPECT_EQ(options.optopt(), 0);
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof((array)[0]))

auto dump_options(ubench::experimental::options& options) -> void {
  int c = 0;
  std::cout << "ARGS: " << options.optstring() << std::endl;
  std::cout << " " << options.prog_name();
  for (const auto& arg : options.args()) {
    std::cout << " " << arg;
  }
  std::cout << std::endl;

  while (c != -1) {
    std::cout << "optind: " << ::optind << std::endl;
    c = options.getopt();
    if (c != -1) {
      std::cout << "  getopt: " << (char)c << std::endl;
      auto arg = options.optarg();
      if (arg.has_value()) {
        std::cout << "  optarg: " << arg.value() << std::endl;
      } else {
        std::cout << "  optarg: (null)" << std::endl;
      }
      std::cout << "  optopt: " << (char)options.optopt() << std::endl;
    }
  }

  std::cout << "PERMUTED ARGS:";
  for (const auto& arg : options.argsreal()) {
    std::cout << " " << arg;
  }
  std::cout << std::endl;
}

TEST(ubench_options_dump, noopts) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "");
  dump_options(options);
}

TEST(ubench_options_dump, oneopt) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f");
  dump_options(options);
}

TEST(ubench_options_dump, oneopt_arg1) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "arg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f");
  dump_options(options);
}

TEST(ubench_options_dump, oneopt_arg2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-farg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f");
  dump_options(options);
}

TEST(ubench_options_dump, oneoptarg_none) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:");
  dump_options(options);
}

TEST(ubench_options_dump, oneoptarg_arg) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-farg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:");
  dump_options(options);
}

TEST(ubench_options_dump, oneoptarg_arg2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "arg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:");
  dump_options(options);
}

TEST(ubench_options_dump, oneoptarg_equals) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f=arg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:");
  dump_options(options);
}

TEST(ubench_options_dump, oneoptarg_alt_none) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, ":f:");
  dump_options(options);
}

TEST(ubench_options_dump, oneoptarg_alt_arg) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-farg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, ":f:");
  dump_options(options);
}

TEST(ubench_options_dump, oneoptarg_alt_arg2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "arg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, ":f:");
  dump_options(options);
}

TEST(ubench_options_dump, twooptsmix) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-farg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, ":f:a");
  dump_options(options);
}

TEST(ubench_options_dump, twooptsmix2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-afarg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, ":f:a");
  dump_options(options);
}

TEST(ubench_options_dump, twooptsmix3) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-a", "-farg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, ":f:a");
  dump_options(options);
}

TEST(ubench_options_dump, twooptsmix4) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-af", "-farg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, ":f:a");
  dump_options(options);
}

TEST(ubench_options_dump, argfirst_noopts) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "arg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f");
  dump_options(options);
}

TEST(ubench_options_dump, argfirst_oneopt) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "arg", "-f"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f");
  dump_options(options);
}

TEST(ubench_options_dump, argfirst2_oneopt) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "arg", "-f", "arg2"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f");
  dump_options(options);
}

TEST(ubench_options_dump, invalidopt) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-z"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  auto options = ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f");
  dump_options(options);
}

TEST(ubench_options_dump, invalidopt_arg) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-z", "arg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f");
  dump_options(options);
}

TEST(ubench_options_dump, invalidopt_char1) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-:", "arg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:");
  dump_options(options);
}

TEST(ubench_options_dump, invalidopt_char2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-<", "arg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:<");
  dump_options(options);
}

TEST(ubench_options_dump, invalidopt_char3) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-@", "arg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:@");
  dump_options(options);
}

TEST(ubench_options_dump, invalidopt_char4) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-;", "arg"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:;");
  dump_options(options);
}

TEST(ubench_options_dump, help) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-?"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "abcf?");
  dump_options(options);
}

TEST(ubench_options_dump, missing_arg_twoargs) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "-a"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:a");
  dump_options(options);
}

TEST(ubench_options_dump, argwhitespace) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f ", "-a"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:a");
  dump_options(options);
}

TEST(ubench_options_dump, minus1) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:a");
  dump_options(options);
}

TEST(ubench_options_dump, minus2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-a", "-"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:a");
  dump_options(options);
}

TEST(ubench_options_dump, minus3) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-", "-a"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:a");
  dump_options(options);
}

TEST(ubench_options_dump, minus4) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "-"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:a");
  dump_options(options);
}

TEST(ubench_options_dump, doubleminus1) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "--"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:a");
  dump_options(options);
}

TEST(ubench_options_dump, doubleminus2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-a", "--"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:a");
  dump_options(options);
}

TEST(ubench_options_dump, doubleminus3) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "--", "-a"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:a");
  dump_options(options);
}

TEST(ubench_options_dump, doubleminus4) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "--"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:a");
  dump_options(options);
}

TEST(ubench_options_dump, longoption1) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "--long"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:a");
  dump_options(options);
}

TEST(ubench_options_dump, longoption2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-f", "--long"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:a");
  dump_options(options);
}

TEST(ubench_options_dump, longoption3) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-a", "--long"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:a");
  dump_options(options);
}

TEST(ubench_options_dump, longoption4) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "--long", "-a"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:a");
  dump_options(options);
}

TEST(ubench_options_dump, vendoropt1) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-a", "-W", "X"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:aW");
  dump_options(options);
}

TEST(ubench_options_dump, vendoropt2) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-W", "-a", "X"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:aW");
  dump_options(options);
}

TEST(ubench_options_dump, vendoropt3) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  const char* const argv[] = {"prog_name", "-a", "-W", "X"};

  auto options =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ubench::experimental::options(ARRAY_LENGTH(argv), argv, "f:aW:");
  dump_options(options);
}
