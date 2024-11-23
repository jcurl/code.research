#include "ubench/args.h"

#include <initializer_list>
#include <string_view>

#include <gtest/gtest.h>

using ::testing::Gt;
using ::testing::Lt;
using ::testing::MatchesRegex;

auto split_check(std::string arg, std::initializer_list<std::string> l)
    -> void {
  auto arg_parse = ubench::args::split_args(arg);
  EXPECT_TRUE(arg_parse);
  if (arg_parse) {
    EXPECT_EQ(arg_parse->size(), l.size());

    std::size_t p = 0;
    for (const auto& a : l) {
      // We're guaranteed that the reference to *arg_parse[p] will work, because
      // a previous check has ensured the optional has a value, and we're
      // indexing within the size which is the same size as the initializer
      // list.
      EXPECT_STREQ((*arg_parse)[p].c_str(), a.c_str());
      p++;
    }
  }
}

TEST(ubench_args, split_args) {
  split_check("", {""});
  split_check("x", {"x"});
  split_check("foo", {"foo"});
  split_check(",", {"", ""});
  split_check("x,", {"x", ""});
  split_check(",y", {"", "y"});
  split_check("x,y", {"x", "y"});
  split_check("x,y,", {"x", "y", ""});
  split_check("x,y,z", {"x", "y", "z"});
  split_check("x,,z", {"x", "", "z"});
  split_check(",y,z", {"", "y", "z"});
}

template <typename T>
auto parse_int_check(std::string arg) -> void {
  auto value = ubench::args::parse_int<T>(arg);
  EXPECT_FALSE(value);
}

template <typename T>
auto parse_int_check(std::string arg, T expected) -> void {
  auto value = ubench::args::parse_int<T>(arg);
  EXPECT_TRUE(value);
  if (value) {
    EXPECT_EQ(value, expected)
        << "Value: " << *value << " didn't result in " << expected;
  }
}

TEST(ubench_args, parse_int) {
  parse_int_check<std::uint8_t>("0", 0);
  parse_int_check<std::uint8_t>("1", 1);
  parse_int_check<std::uint8_t>("255", 255);
  parse_int_check<std::uint8_t>("256");
  parse_int_check<std::uint8_t>("-1");
  parse_int_check<std::uint8_t>("0x");

  parse_int_check<std::int8_t>("0", 0);
  parse_int_check<std::int8_t>("1", 1);
  parse_int_check<std::int8_t>("127", 127);
  parse_int_check<std::int8_t>("128");
  parse_int_check<std::int8_t>("-1", -1);
  parse_int_check<std::int8_t>("-128", -128);
  parse_int_check<std::int8_t>("0x");

  parse_int_check<int>("1.0");
  parse_int_check<int>("-1", -1);
  parse_int_check<int>("x");
}
