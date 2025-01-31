#include <string>
#include <type_traits>

#include <gtest/gtest.h>

#include "stdext/expected.h"

TEST(expected, void_and_then_value) {
  using T = std::string;
  using Expected = stdext::expected<int, T>;
  using Expected2 = stdext::expected<void, T>;

  Expected e = 42;
  const auto newVal = e.and_then([](int) -> Expected2 { return Expected2{}; });
  auto result = std::is_same_v<std::remove_cv_t<decltype(newVal)>, Expected2>;
  EXPECT_TRUE(result);
  EXPECT_TRUE(newVal);
}

TEST(expected, nonvoid_and_then_value) {
  using T = std::string;
  using Expected = stdext::expected<int, T>;
  using Expected2 = stdext::expected<double, T>;

  Expected e = 42;
  const auto newVal = e.and_then([](int x) { return Expected2{x * 2}; });
  auto result = std::is_same_v<std::remove_cv_t<decltype(newVal)>, Expected2>;
  EXPECT_TRUE(result);
  EXPECT_TRUE(newVal);
}

TEST(expected, void_and_then_expected) {
  using T = std::string;
  using Expected = stdext::expected<void, T>;
  using Expected2 = stdext::expected<void, T>;

  Expected e{};
  const auto newVal = e.and_then([]() -> Expected2 { return Expected2{}; });
  auto result = std::is_same_v<std::remove_cv_t<decltype(newVal)>, Expected2>;
  EXPECT_TRUE(result);
  EXPECT_TRUE(newVal);
}

TEST(expected, nonvoid_and_then) {
  using T = std::string;
  using Expected = stdext::expected<void, T>;
  using Expected2 = stdext::expected<double, T>;

  Expected e{};
  const auto newVal = e.and_then([]() { return Expected2{}; });
  auto result = std::is_same_v<std::remove_cv_t<decltype(newVal)>, Expected2>;
  EXPECT_TRUE(result);
  EXPECT_TRUE(newVal);
}

TEST(expected, or_else) {
  using T = std::string;
  using Expected = stdext::expected<T, int>;
  using Expected2 = stdext::expected<T, double>;

  Expected e{};
  const auto newVal = e.or_else([](auto&&) { return Expected2{}; });
  auto result = std::is_same_v<std::remove_cv_t<decltype(newVal)>, Expected2>;
  EXPECT_TRUE(result);
  EXPECT_TRUE(newVal);
}

TEST(expected, void_or_else) {
  using Expected = stdext::expected<void, int>;
  using Expected2 = stdext::expected<void, double>;

  Expected e{};
  const auto newVal = e.or_else([](auto&&) { return Expected2{}; });
  auto result = std::is_same_v<std::remove_cv_t<decltype(newVal)>, Expected2>;
  EXPECT_TRUE(result);
  EXPECT_TRUE(newVal);
}
