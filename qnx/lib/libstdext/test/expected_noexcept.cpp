#include <gtest/gtest.h>

#include "stdext/expected.h"

struct FromType {};

enum class NoThrowConvertible {
  Yes,
  No,
};

template <NoThrowConvertible N>
struct ToType;

template <>
struct ToType<NoThrowConvertible::Yes> {
  ToType() = default;
  ToType(const ToType&) = default;
  auto operator=(const ToType&) -> ToType& = default;
  ToType(ToType&&) = default;
  auto operator=(ToType&&) -> ToType& = default;
  ~ToType() = default;

  ToType(const FromType&) noexcept {}
};

template <>
struct ToType<NoThrowConvertible::No> {
  ToType() = default;
  ToType(const ToType&) = default;
  auto operator=(const ToType&) -> ToType& = default;
  ToType(ToType&&) = default;
  auto operator=(ToType&&) -> ToType& = default;
  ~ToType() = default;

  ToType(const FromType&) noexcept(false) {}
};

TEST(expect, value_or_noexcept_true) {
  using T = ToType<NoThrowConvertible::Yes>;
  using E = int;
  using Expected = stdext::expected<T, E>;
  Expected e;
  EXPECT_TRUE(noexcept(e.value_or(FromType{})));
}

TEST(expect, value_or_noexcept_false) {
  using T = ToType<NoThrowConvertible::No>;
  using E = int;
  using Expected = stdext::expected<T, E>;
  Expected e;
  EXPECT_FALSE(noexcept(e.value_or(FromType{})));
}

TEST(expect, error_or_noexcept_true) {
  using T = int;
  using E = ToType<NoThrowConvertible::Yes>;
  using Expected = stdext::expected<T, E>;
  Expected e;
  EXPECT_TRUE(noexcept(e.error_or(FromType{})));
}

TEST(expect, error_or_noexcept_false) {
  using T = int;
  using E = ToType<NoThrowConvertible::No>;
  using Expected = stdext::expected<T, E>;
  Expected e;
  EXPECT_FALSE(noexcept(e.error_or(FromType{})));
}

TEST(expect, void_error_or_noexcept_true) {
  using T = void;
  using E = ToType<NoThrowConvertible::Yes>;
  using Expected = stdext::expected<T, E>;
  Expected e;
  EXPECT_TRUE(noexcept(e.error_or(FromType{})));
}

TEST(expect, void_error_or_noexcept_false) {
  using T = void;
  using E = ToType<NoThrowConvertible::No>;
  using Expected = stdext::expected<T, E>;
  Expected e;
  EXPECT_FALSE(noexcept(e.error_or(FromType{})));
}
