#include <string>

#include <gtest/gtest.h>

#include "stdext/expected.h"

namespace {

template <bool ShouldEqual, typename T, typename U>
constexpr auto EqualityTester(const T& lhs, const U& rhs) -> bool {
  return (lhs == rhs) == ShouldEqual && (lhs != rhs) != ShouldEqual &&
         (rhs == lhs) == ShouldEqual && (rhs != lhs) != ShouldEqual;
}

struct Type1 {
  std::string value;

  Type1(std::string v) : value(std::move(v)) {}
  Type1(const char* v) : value(v) {}
  Type1(const Type1&) = delete;
  auto operator=(const Type1&) -> Type1& = delete;
  Type1(Type1&& other) = default;
  auto operator=(Type1&&) -> Type1& = default;
  ~Type1() = default;

  auto operator==(const Type1& rhs) const -> bool { return value == rhs.value; }
};

struct Type2 {
  std::string value;

  Type2(std::string v) : value(std::move(v)) {}
  Type2(const char* v) : value(v) {}
  Type2(const Type2&) = delete;
  auto operator=(const Type2&) -> Type2& = delete;
  Type2(Type2&& other) = default;
  auto operator=(Type2&&) -> Type2& = default;
  ~Type2() = default;

  auto operator==(const Type2& rhs) const -> bool { return value == rhs.value; }
};

inline auto operator==(const Type1& lhs, const Type2& rhs) -> bool {
  return lhs.value == rhs.value;
}

inline auto operator==(const Type2& lhs, const Type1& rhs) -> bool {
  return rhs == lhs;
}

}  // namespace

TEST(expected, t_is_not_void_compare_with_same_type) {
  using T = Type1;
  using E = Type2;
  using Expected = stdext::expected<T, E>;

  const Expected value1 = "value1";
  const Expected value2 = "value2";
  const Expected value1Copy = "value1";
  const Expected error1 = stdext::unexpected<E>("error1");
  const Expected error2 = stdext::unexpected<E>("error2");
  const Expected error1Copy = stdext::unexpected<E>("error1");

  EXPECT_TRUE(EqualityTester<true>(value1, value1Copy));
  EXPECT_TRUE(EqualityTester<false>(value1, value2));
  EXPECT_TRUE(EqualityTester<true>(error1, error1Copy));
  EXPECT_TRUE(EqualityTester<false>(error1, error2));
  EXPECT_TRUE(EqualityTester<false>(value1, error1));
}

TEST(expected, t_is_not_void_compare_with_different_type) {
  using T = Type1;
  using E = Type2;
  using Expected = stdext::expected<T, E>;

  using T2 = Type2;
  using E2 = Type1;
  static_assert(!std::is_same_v<T, T2>);
  static_assert(!std::is_same_v<E, E2>);
  using Expected2 = stdext::expected<T2, E2>;

  const Expected value1 = "value1";
  const Expected2 value2 = "value2";
  const Expected2 value1Same = "value1";
  const Expected error1 = stdext::unexpected<E>("error1");
  const Expected2 error2 = stdext::unexpected<E2>("error2");
  const Expected2 error1Same = stdext::unexpected<E2>("error1");

  EXPECT_TRUE(EqualityTester<true>(value1, value1Same));
  EXPECT_TRUE(EqualityTester<false>(value1, value2));
  EXPECT_TRUE(EqualityTester<true>(error1, error1Same));
  EXPECT_TRUE(EqualityTester<false>(error1, error2));
  EXPECT_TRUE(EqualityTester<false>(value1, error1));
}

TEST(expected, t_is_not_void_compare_with_same_value_type) {
  using T = Type1;
  using E = Type2;
  using Expected = stdext::expected<T, E>;

  const Expected value1 = "value1";
  const Expected error1 = stdext::unexpected<E>("error1");
  const T value2 = "value2";
  const T value1Same = "value1";

  EXPECT_TRUE(EqualityTester<true>(value1, value1Same));
  EXPECT_TRUE(EqualityTester<false>(value1, value2));
  EXPECT_TRUE(EqualityTester<false>(error1, value2));
}

TEST(expected, t_is_not_void_compare_with_same_value_type2) {
  using T = Type1;
  using E = Type2;
  using Expected = stdext::expected<T, E>;

  using T2 = Type2;
  static_assert(!std::is_same_v<T, T2>);

  const Expected value1 = "value1";
  const Expected error1 = stdext::unexpected<E>("error1");
  const T2 value2 = "value2";
  const T2 value1Same = "value1";

  EXPECT_TRUE(EqualityTester<true>(value1, value1Same));
  EXPECT_TRUE(EqualityTester<false>(value1, value2));
  EXPECT_TRUE(EqualityTester<false>(error1, value2));
}

TEST(expected, t_is_not_void_compare_with_same_error_type) {
  using T = Type1;
  using E = Type2;
  using Expected = stdext::expected<T, E>;

  const Expected value1 = "value1";
  const Expected error1 = stdext::unexpected<E>("error1");
  const auto error2 = stdext::unexpected<E>("error2");
  const auto error1Same = stdext::unexpected<E>("error1");

  EXPECT_TRUE(EqualityTester<true>(error1, error1Same));
  EXPECT_TRUE(EqualityTester<false>(error1, error2));
  EXPECT_TRUE(EqualityTester<false>(value1, error2));
}

TEST(expected, t_is_not_void_compare_with_different_error_type) {
  using T = Type1;
  using E = Type2;
  using Expected = stdext::expected<T, E>;

  using E2 = Type1;
  static_assert(!std::is_same_v<E, E2>);

  const Expected value1 = "value1";
  const Expected error1 = stdext::unexpected<E>("error1");
  const auto error2 = stdext::unexpected<E2>("error2");
  const auto error1Same = stdext::unexpected<E2>("error1");

  EXPECT_TRUE(EqualityTester<true>(error1, error1Same));
  EXPECT_TRUE(EqualityTester<false>(error1, error2));
  EXPECT_TRUE(EqualityTester<false>(value1, error2));
}

TEST(expected, t_is_void_compare_with_same_type) {
  using E = Type1;
  using Expected = stdext::expected<void, E>;

  const Expected value1;
  const Expected value2;
  const Expected error1 = stdext::unexpected<E>("error1");
  const Expected error2 = stdext::unexpected<E>("error2");
  const Expected error1Copy = stdext::unexpected<E>("error1");

  EXPECT_TRUE(EqualityTester<true>(value1, value2));
  EXPECT_TRUE(EqualityTester<true>(error1, error1Copy));
  EXPECT_TRUE(EqualityTester<false>(error1, error2));
  EXPECT_TRUE(EqualityTester<false>(value1, error1));
}

TEST(expected, t_is_void_compare_with_different_type) {
  using E = Type1;
  using Expected = stdext::expected<void, E>;

  using E2 = Type2;
  static_assert(!std::is_same_v<E, E2>);
  using Expected2 = stdext::expected<void, E2>;

  const Expected value1;
  const Expected2 value2;
  const Expected error1 = stdext::unexpected<E>("error1");
  const Expected2 error2 = stdext::unexpected<E2>("error2");
  const Expected2 error1Same = stdext::unexpected<E2>("error1");

  EXPECT_TRUE(EqualityTester<true>(value1, value2));
  EXPECT_TRUE(EqualityTester<true>(error1, error1Same));
  EXPECT_TRUE(EqualityTester<false>(error1, error2));
  EXPECT_TRUE(EqualityTester<false>(value1, error1));
}

TEST(expected, t_is_void_compare_with_same_error_type) {
  using E = Type1;
  using Expected = stdext::expected<void, E>;

  const Expected value1;
  const Expected error1 = stdext::unexpected<E>("error1");
  const auto error2 = stdext::unexpected<E>("error2");
  const auto error1Same = stdext::unexpected<E>("error1");

  EXPECT_TRUE(EqualityTester<true>(error1, error1Same));
  EXPECT_TRUE(EqualityTester<false>(error1, error2));
  EXPECT_TRUE(EqualityTester<false>(value1, error1));
}

TEST(expected, t_is_void_compare_with_different_error_type) {
  using E = Type1;
  using Expected = stdext::expected<void, E>;

  using E2 = Type2;
  static_assert(!std::is_same_v<E, E2>);

  const Expected value1;
  const Expected error1 = stdext::unexpected<E>("error1");
  const auto error2 = stdext::unexpected<E2>("error2");
  const auto error1Same = stdext::unexpected<E2>("error1");

  EXPECT_TRUE(EqualityTester<true>(error1, error1Same));
  EXPECT_TRUE(EqualityTester<false>(error1, error2));
  EXPECT_TRUE(EqualityTester<false>(value1, error2));
}
