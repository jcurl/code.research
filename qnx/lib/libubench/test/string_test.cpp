#include "ubench/string.h"

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <vector>

#include <gtest/gtest.h>

auto split_check(const std::string& arg, std::initializer_list<std::string> l)
    -> void {
  auto arg_parse = ubench::string::split_args(arg);
  EXPECT_EQ(arg_parse.size(), l.size());

  // The output is of `string_view`. For checking, we convert to a `string`.
  std::vector<std::string> args{};
  std::transform(arg_parse.cbegin(), arg_parse.cend(), std::back_inserter(args),
      [](std::string_view x) { return std::string{x}; });

  std::size_t p = 0;
  for (const auto& a : l) {
    // Possibility 1: Do the conversion in the loop.
    std::string v{arg_parse[p]};
    EXPECT_STREQ(v.c_str(), a.c_str());

    // Possibility 2: We did a transform earlier. Useful if 'arg' would go out
    // of scope and we return a new vector.
    EXPECT_STREQ(args[p].c_str(), a.c_str());

    // Possibility 3: Use it directly, while it is in scope.
    EXPECT_TRUE(arg_parse[p] == a);
    p++;
  }
}

TEST(ubench_string, split_args) {
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

template<typename T>
auto split_check_int(const std::string& arg, std::initializer_list<T> l) -> void {
  auto arg_parse = ubench::string::split_args_int<T>(arg);
  ASSERT_EQ(arg_parse.size(), l.size());

  std::size_t p = 0;
  for (const auto& a : l) {
    EXPECT_EQ(arg_parse[p], a);
    p++;
  }
}

TEST(ubench_string, split_args_int) {
  split_check_int<int>("", { });
  split_check_int<int>("1", { 1 });
  split_check_int<int>("1,", { });
  split_check_int<int>("1,2", { 1, 2 });
  split_check_int<int>("1,-22", { 1, -22 });
  split_check_int<int>("-11,2", { -11, 2 });
  split_check_int<int>("a", { });
  split_check_int<int>("1,a", { });
  split_check_int<int>("a,1", { });

  split_check_int<unsigned int>("", { });
  split_check_int<unsigned int>("1", { 1 });
  split_check_int<unsigned int>("1,", { });
  split_check_int<unsigned int>("1,2", { 1, 2 });
  split_check_int<unsigned int>("1,-22", { });
  split_check_int<unsigned int>("-11,2", { });
  split_check_int<unsigned int>("a", { });
  split_check_int<unsigned int>("1,a", { });
  split_check_int<unsigned int>("a,1", { });
}

auto split_check_max_fields(
    const std::string& arg, std::initializer_list<std::string> l) -> void {
  auto arg_parse = ubench::string::split_args(arg, l.size());
  ASSERT_EQ(arg_parse.size(), l.size());

  // The output is of `string_view`. For checking, we convert to a `string`.
  std::vector<std::string> args{};
  std::transform(arg_parse.cbegin(), arg_parse.cend(), std::back_inserter(args),
      [](std::string_view x) { return std::string{x}; });

  std::size_t p = 0;
  for (const auto& a : l) {
    // Possibility 1: Do the conversion in the loop.
    std::string v{arg_parse[p]};
    EXPECT_STREQ(v.c_str(), a.c_str());

    // Possibility 2: We did a transform earlier. Useful if 'arg' would go out
    // of scope and we return a new vector.
    EXPECT_STREQ(args[p].c_str(), a.c_str());

    // Possibility 3: Use it directly, while it is in scope.
    EXPECT_TRUE(arg_parse[p] == a);
    p++;
  }
}

TEST(ubench_string, split_args_fields) {
  // Repeat the original tests, but now with the exact fields given.
  split_check_max_fields("", {""});
  split_check_max_fields("x", {"x"});
  split_check_max_fields("foo", {"foo"});
  split_check_max_fields(",", {"", ""});
  split_check_max_fields("x,", {"x", ""});
  split_check_max_fields(",y", {"", "y"});
  split_check_max_fields("x,y", {"x", "y"});
  split_check_max_fields("x,y,", {"x", "y", ""});
  split_check_max_fields("x,y,z", {"x", "y", "z"});
  split_check_max_fields("x,,z", {"x", "", "z"});
  split_check_max_fields(",y,z", {"", "y", "z"});

  // If we have a limit. The last field will contain the remaining items.
  split_check_max_fields("x,y,", {"x", "y,"});
  split_check_max_fields("x,y,z", {"x", "y,z"});
}

template <typename T>
auto parse_int_check(const std::string& arg) -> void {
  auto value = ubench::string::parse_int<T>(arg);
  EXPECT_FALSE(value);
}

template <typename T>
auto parse_int_check(const std::string& arg, T expected) -> void {
  auto value = ubench::string::parse_int<T>(arg);
  EXPECT_TRUE(value);
  if (value) {
    EXPECT_EQ(value, expected)
        << "Value: " << *value << " didn't result in " << expected;
  }
}

TEST(ubench_string, parse_int) {
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

static const std::string byte_string{"12"};
static const std::string short_string{"fa23"};
static const std::string long_string{"de45ca28"};
static const std::string llong_string{"87ea24c26e7b3d21"};
static const std::string xlong_string{"26ef1ac49067fe375e2b1ac98C58f10d"};

template <typename T>
auto from_chars_hex_check(const std::string& arg, T expected) -> void {
  T value;
  auto [ptr, ec] = ubench::string::from_chars_hex(
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      arg.data(), arg.data() + arg.size(), value);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  EXPECT_EQ(ptr, arg.data() + arg.size())
      << arg << ", " << std::hex << expected;
  EXPECT_EQ(ec, std::errc{}) << arg << ", " << std::hex << expected;
  EXPECT_EQ(value, expected) << arg << ", " << std::hex << expected;
}

TEST(ubench_string, from_chars_hex) {
  from_chars_hex_check<std::uint8_t>(byte_string, 0x12);
  from_chars_hex_check<std::uint8_t>(short_string, 0x23);
  from_chars_hex_check<std::uint8_t>(long_string, 0x28);
  from_chars_hex_check<std::uint8_t>(llong_string, 0x21);
  from_chars_hex_check<std::uint8_t>(xlong_string, 0x0d);

  from_chars_hex_check<std::uint16_t>(byte_string, 0x12);
  from_chars_hex_check<std::uint16_t>(short_string, 0xfa23);
  from_chars_hex_check<std::uint16_t>(long_string, 0xca28);
  from_chars_hex_check<std::uint16_t>(llong_string, 0x3d21);
  from_chars_hex_check<std::uint16_t>(xlong_string, 0xf10d);

  from_chars_hex_check<std::uint32_t>(byte_string, 0x12);
  from_chars_hex_check<std::uint32_t>(short_string, 0xfa23);
  from_chars_hex_check<std::uint32_t>(long_string, 0xde45ca28);
  from_chars_hex_check<std::uint32_t>(llong_string, 0x6e7b3d21);
  from_chars_hex_check<std::uint32_t>(xlong_string, 0x8c58f10d);

  from_chars_hex_check<std::uint64_t>(byte_string, 0x12);
  from_chars_hex_check<std::uint64_t>(short_string, 0xfa23);
  from_chars_hex_check<std::uint64_t>(long_string, 0xde45ca28);
  from_chars_hex_check<std::uint64_t>(llong_string, 0x87ea24c26e7b3d21);
  from_chars_hex_check<std::uint64_t>(xlong_string, 0x5e2b1ac98c58f10d);

  from_chars_hex_check<std::int8_t>(byte_string, 0x12);
  from_chars_hex_check<std::int8_t>(short_string, 0x23);
  from_chars_hex_check<std::int8_t>(long_string, 0x28);
  from_chars_hex_check<std::int8_t>(llong_string, 0x21);
  from_chars_hex_check<std::int8_t>(xlong_string, 0x0d);

  from_chars_hex_check<std::int16_t>(byte_string, 0x12);
  from_chars_hex_check<std::int16_t>(short_string, -1501);
  from_chars_hex_check<std::int16_t>(long_string, -13784);
  from_chars_hex_check<std::int16_t>(llong_string, 0x3d21);
  from_chars_hex_check<std::int16_t>(xlong_string, -3827);

  from_chars_hex_check<std::int32_t>(byte_string, 0x12);
  from_chars_hex_check<std::int32_t>(short_string, 0xfa23);
  from_chars_hex_check<std::int32_t>(long_string, -565851608);
  from_chars_hex_check<std::int32_t>(llong_string, 0x6e7b3d21);
  from_chars_hex_check<std::int32_t>(xlong_string, -1940328179);

  from_chars_hex_check<std::int64_t>(byte_string, 0x12);
  from_chars_hex_check<std::int64_t>(short_string, 0xfa23);
  from_chars_hex_check<std::int64_t>(long_string, 0xde45ca28);
  from_chars_hex_check<std::int64_t>(llong_string, -8653063316543161055);
  from_chars_hex_check<std::int64_t>(xlong_string, 0x5e2b1ac98c58f10d);
}

static const std::string inv_string1{"0x12"};
static const std::string inv_string2{"fa23\1"};
static const std::string inv_string3{"de45\377ca28"};
static const std::string inv_string4{"87ea2z4c26e7yb3d21"};
static const std::string inv_string5{
    "87ea24c26e76e2ab3d2\x80"
    "1"};

template <typename T>
auto from_chars_hex_check_error(const std::string& arg, size_t len, T expected)
    -> void {
  T value;
  auto [ptr, ec] = ubench::string::from_chars_hex(
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      arg.data(), arg.data() + arg.size(), value);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  EXPECT_EQ(ptr, arg.data() + len) << arg << ", " << std::hex << expected;
  EXPECT_EQ(ec, std::errc::invalid_argument)
      << arg << ", " << std::hex << expected;
  EXPECT_EQ(value, expected) << arg << ", " << std::hex << expected;
}

TEST(ubench_string, from_chars_hex_invalid) {
  from_chars_hex_check_error<std::uint8_t>(inv_string1, 1, 0);
  from_chars_hex_check_error<std::uint8_t>(inv_string2, 4, 0x23);
  from_chars_hex_check_error<std::uint8_t>(inv_string3, 4, 0x45);
  from_chars_hex_check_error<std::uint8_t>(inv_string4, 5, 0xa2);
  from_chars_hex_check_error<std::uint8_t>(inv_string5, 19, 0xd2);

  from_chars_hex_check_error<std::uint16_t>(inv_string1, 1, 0);
  from_chars_hex_check_error<std::uint16_t>(inv_string2, 4, 0xfa23);
  from_chars_hex_check_error<std::uint16_t>(inv_string3, 4, 0xde45);
  from_chars_hex_check_error<std::uint16_t>(inv_string4, 5, 0x7ea2);
  from_chars_hex_check_error<std::uint16_t>(inv_string5, 19, 0xb3d2);

  from_chars_hex_check_error<std::uint32_t>(inv_string1, 1, 0);
  from_chars_hex_check_error<std::uint32_t>(inv_string2, 4, 0xfa23);
  from_chars_hex_check_error<std::uint32_t>(inv_string3, 4, 0xde45);
  from_chars_hex_check_error<std::uint32_t>(inv_string4, 5, 0x87ea2);
  from_chars_hex_check_error<std::uint32_t>(inv_string5, 19, 0x6e2ab3d2);

  from_chars_hex_check_error<std::uint64_t>(inv_string1, 1, 0);
  from_chars_hex_check_error<std::uint64_t>(inv_string2, 4, 0xfa23);
  from_chars_hex_check_error<std::uint64_t>(inv_string3, 4, 0xde45);
  from_chars_hex_check_error<std::uint64_t>(inv_string4, 5, 0x87ea2);
  from_chars_hex_check_error<std::uint64_t>(
      inv_string5, 19, 0xa24c26e76e2ab3d2);

  from_chars_hex_check_error<std::int8_t>(inv_string1, 1, 0);
  from_chars_hex_check_error<std::int8_t>(inv_string2, 4, 0x23);
  from_chars_hex_check_error<std::int8_t>(inv_string3, 4, 0x45);
  from_chars_hex_check_error<std::int8_t>(inv_string4, 5, -94);
  from_chars_hex_check_error<std::int8_t>(inv_string5, 19, -46);

  from_chars_hex_check_error<std::int16_t>(inv_string1, 1, 0);
  from_chars_hex_check_error<std::int16_t>(inv_string2, 4, -1501);
  from_chars_hex_check_error<std::int16_t>(inv_string3, 4, -8635);
  from_chars_hex_check_error<std::int16_t>(inv_string4, 5, 0x7ea2);
  from_chars_hex_check_error<std::int16_t>(inv_string5, 19, -19502);

  from_chars_hex_check_error<std::int32_t>(inv_string1, 1, 0);
  from_chars_hex_check_error<std::int32_t>(inv_string2, 4, 0xfa23);
  from_chars_hex_check_error<std::int32_t>(inv_string3, 4, 0xde45);
  from_chars_hex_check_error<std::int32_t>(inv_string4, 5, 0x87ea2);
  from_chars_hex_check_error<std::int32_t>(inv_string5, 19, 0x6e2ab3d2);

  from_chars_hex_check_error<std::int64_t>(inv_string1, 1, 0);
  from_chars_hex_check_error<std::int64_t>(inv_string2, 4, 0xfa23);
  from_chars_hex_check_error<std::int64_t>(inv_string3, 4, 0xde45);
  from_chars_hex_check_error<std::int64_t>(inv_string4, 5, 0x87ea2);
  from_chars_hex_check_error<std::int64_t>(
      inv_string5, 19, -6751978965907622958);
}