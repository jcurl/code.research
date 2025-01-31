#include <optional>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

#include "stdext/expected.h"

TEST(expected, default_constructable) {
  stdext::expected<int, int> e1;
  stdext::expected<int, int> e2{};
  EXPECT_TRUE(e1);
  EXPECT_TRUE(e2);
  EXPECT_EQ(e1.value(), 0);
  EXPECT_EQ(e2.value(), 0);
}

TEST(expected, default_not_constructable) {
  struct NotDefaultConstructable {
    NotDefaultConstructable() = delete;
  };

  // should not compiled
  EXPECT_FALSE(std::is_trivially_constructible_v<NotDefaultConstructable>);
}

TEST(expected, construct_e_in_place) {
  stdext::expected<void, int> e1{stdext::unexpect};
  stdext::expected<void, int> e2{stdext::unexpect, 42};
  stdext::expected<int, int> e3{stdext::unexpect};
  stdext::expected<int, int> e4{stdext::unexpect, 42};

  EXPECT_FALSE(e1);
  EXPECT_FALSE(e2);
  EXPECT_FALSE(e3);
  EXPECT_FALSE(e4);
}

TEST(expected, implicit_conversion_from_t) {
  stdext::expected<int, int> e = 42;
  EXPECT_TRUE(e);
  EXPECT_EQ(e.value(), 42);
}

TEST(expected, implicit_conversion_from_e) {
  stdext::expected<std::string, int> e1 = stdext::unexpected<int>(42);
  stdext::expected<std::string, int> e2{stdext::unexpect, 42};
  EXPECT_FALSE(e1);
  EXPECT_FALSE(e2);
  EXPECT_EQ(e1.error(), 42);
  EXPECT_EQ(e2.error(), 42);
}

TEST(expected, implicit_conversion_from_e_2) {
  auto result = std::is_convertible_v<stdext::expected<std::string, int>, int>;
  EXPECT_FALSE(result);
}

// TEST(expected, expected_t_void_e) {
//   // should not compiled
//   stdext::expected<int, void> e = 42;
// }

TEST(expected, expected_void_e_t) {
  stdext::expected<void, int> e1 = {};
  stdext::expected<void, int> e2 = stdext::unexpected(42);
  EXPECT_TRUE(e1);
  EXPECT_FALSE(e2);
}

TEST(expected, expected_void_e_t_2) {
  stdext::expected<void, int> e;
  EXPECT_TRUE(e.has_value());

  auto result = std::is_same_v<decltype(e.value()), void>;
  EXPECT_TRUE(result);
}

TEST(expected, emplace_t) {
  struct S {
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    S(int i, double d) noexcept : i(i), d(d) {}
    int i;
    double d;
  };
  stdext::expected<S, std::nullopt_t> e{stdext::unexpect, std::nullopt};
  e.emplace(42, 3.14);
  EXPECT_TRUE(e);
  EXPECT_EQ(e.value().i, 42);
  EXPECT_EQ(e.value().d, 3.14);
}

// TEST(expect, never_empty) {
//   class ThrowOnDefaultConstruct {
//    public:
//     ThrowOnDefaultConstruct() noexcept(false) {
//       throw std::runtime_error("ThrowOnDefaultConstruct");
//     }
//   };
//   stdext::expected<ThrowOnDefaultConstruct, std::nullopt_t> e1{
//       stdext::unexpect, std::nullopt};
//
//   // Okay, should not compiled
//   e1.emplace();
// }

TEST(expected, equality) {
  const stdext::expected<int, int> v1;
  const stdext::expected<int, int> v2{42};
  const stdext::expected<int, int> v3 = 42;
  const stdext::expected<int, int> e1{stdext::unexpect, 0};
  const stdext::expected<int, int> e2{stdext::unexpect, 42};
  const stdext::expected<int, int> e3 = stdext::unexpected(42);
  EXPECT_NE(v1, v2);
  EXPECT_EQ(v2, v3);
  EXPECT_NE(v1, e1);
  EXPECT_NE(v1, e2);
  EXPECT_NE(e1, e2);
  EXPECT_EQ(e2, e3);
  EXPECT_NE(e1, v1);
  EXPECT_NE(e1, v2);
}

TEST(expected, unexpected_error) {
  auto e = stdext::unexpected(42);
  EXPECT_EQ(e.error(), 42);
}

TEST(expected, bad_expected_access_error) {
  stdext::expected<void, int> e = stdext::unexpected(42);
  try {
    e.value();
    EXPECT_TRUE(false);
  } catch (const stdext::bad_expected_access<int>& ex) {
    EXPECT_EQ(ex.error(), 42);
  }
}

TEST(expected, expected_void_e_emplace) {
  stdext::expected<void, int> e1{std::in_place};
  stdext::expected<void, int> e2;
  e2.emplace();
  EXPECT_TRUE(e1);
  EXPECT_TRUE(e2);
}

TEST(expected, swap) {
  stdext::expected<int, int> e1{42};
  stdext::expected<int, int> e2{1337};

  e1.swap(e2);
  EXPECT_EQ(e1.value(), 1337);
  EXPECT_EQ(e2.value(), 42);

  swap(e1, e2);
  EXPECT_EQ(e1.value(), 42);
  EXPECT_EQ(e2.value(), 1337);
}

TEST(expected, copy_initialization) {
  stdext::expected<std::string, int> e = {};
  EXPECT_TRUE(e);
}

TEST(expected, brace_constructed_value) {
  {  // non-void
    stdext::expected<int, int> e{{}};
    EXPECT_TRUE(e);
  }
  {  // std::string
#if !defined(__GNUC__)
     // This case doesn't work and needs clarification:
    //   * an implementation bug
    //   + a GCC bug?
    //   + a language bug?
    //   + a MSVC bug?
    // see CWG-1228(NAD), CWG-2735(DR), CWG-2856(DR)
    // see https://github.com/cplusplus/CWG/issues/486
    // see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59389
    // see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60027
    // see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109247
    // see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=113300
    stdext::expected<std::string, int> e{{}};
    EXPECT_TRUE(e);
#endif
  }
}

TEST(expected, lwg_3836) {
  struct BaseError {};
  struct DerivedError : BaseError {};

  stdext::expected<bool, DerivedError> e1(false);
  stdext::expected<bool, BaseError> e2(e1);
  EXPECT_FALSE(e2.value());

  stdext::expected<void, DerivedError> e3{};
  stdext::expected<void, BaseError> e4(e3);
  EXPECT_TRUE(e4.has_value());
}

TEST(expected, assignment) {
  // non-void
  {// error = value
      {stdext::expected<int, int> e1{stdext::unexpect, 1337};
  stdext::expected<int, int> e2{42};

  e1 = e2;
  EXPECT_TRUE(e1);
  EXPECT_EQ(e1, e2);
}
}

// void
{
  // error = value
  {
    stdext::expected<void, int> e1{stdext::unexpect, 1337};
    stdext::expected<void, int> e2{};

    e1 = e2;
    EXPECT_TRUE(e1);
    EXPECT_EQ(e1, e2);
  }
}
}

TEST(expected, user_provided_move_tlgh97) {
  // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
  struct user_provided_move {
    user_provided_move() = default;
    user_provided_move(user_provided_move&&) noexcept {}
  };
  using Expected = stdext::expected<user_provided_move, int>;
  Expected t1;
  Expected tm1(std::move(t1));
  EXPECT_TRUE(tm1);
}

TEST(expected, user_provided_defaulted_move_tlgh97) {
  // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
  struct defaulted_move {
    defaulted_move() = default;
    defaulted_move(defaulted_move&&) = default;
  };
  using Expected = stdext::expected<defaulted_move, int>;
  Expected t2;
  Expected tm2(std::move(t2));  // should compile
  EXPECT_TRUE(tm2);
}

TEST(expected, move_only_tlgh145) {
  // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
  class MoveOnly {
   public:
    MoveOnly() = default;

    // Non-copyable
    MoveOnly(const MoveOnly&) = delete;
    auto operator=(const MoveOnly&) -> MoveOnly& = delete;

    // Movable trivially
    MoveOnly(MoveOnly&&) = default;
    auto operator=(MoveOnly&&) -> MoveOnly& = default;
  };

  using Expected = stdext::expected<MoveOnly, int>;
  Expected a{};
  Expected b = std::move(a);  // should compile

  EXPECT_TRUE(b);
}
