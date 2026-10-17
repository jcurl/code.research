#include <cerrno>
#include <optional>
#include <string>

#include <gtest/gtest-spi.h>

#include "rtest/rtest.h"
#include "stdext/expected.h"

TEST(optional, have_optional_missing_value) {
  static std::optional<int> none = std::nullopt;
  ASSERT_FALSE(none);
}

TEST(optional, have_optional_present) {
  std::optional<int> present = 1;
  ASSERT_OPTIONAL(present);

  if (*present == 1) std::cout << "present value" << std::endl;
  ASSERT_EQ(*present, 1);
}

TEST(optional, assert_optional_missing_value) {
  static std::optional<int> none;
  EXPECT_FATAL_FAILURE(ASSERT_OPTIONAL(none), "Expected: true");
}

// Counts evaluations of the streamed message. Global because the statement
// given to EXPECT_*_FAILURE is compiled in a helper class that cannot capture
// locals.

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
int streamed_detail_evaluations = 0;

auto streamed_detail() -> const char* {
  ++streamed_detail_evaluations;
  return "streamed detail";
}

TEST(optional, streamed_message_reported_when_value_missing) {
  static std::optional<bool> none;
  streamed_detail_evaluations = 0;
  EXPECT_FATAL_FAILURE(
      ASSERT_OPTIONAL_TRUE(none) << streamed_detail(), "streamed detail");
  EXPECT_EQ(streamed_detail_evaluations, 1);
}

TEST(optional, streamed_message_not_evaluated_when_value_wrong) {
  static std::optional<bool> f = false;
  streamed_detail_evaluations = 0;
  EXPECT_NONFATAL_FAILURE(
      EXPECT_OPTIONAL_TRUE(f) << streamed_detail(), "optional with true value");
  EXPECT_EQ(streamed_detail_evaluations, 0);
}

TEST(optional, assert_optional_true) {
  static std::optional<bool> t = true;
  static std::optional<bool> f = false;
  static std::optional<bool> none;
  ASSERT_OPTIONAL_TRUE(t);
  EXPECT_FATAL_FAILURE(ASSERT_OPTIONAL_TRUE(f), "optional with true value");
  EXPECT_FATAL_FAILURE(ASSERT_OPTIONAL_TRUE(none), "optional with true value");
}

TEST(optional, assert_optional_false) {
  static std::optional<bool> t = true;
  static std::optional<bool> f = false;
  static std::optional<bool> none;
  ASSERT_OPTIONAL_FALSE(f);
  EXPECT_FATAL_FAILURE(ASSERT_OPTIONAL_FALSE(t), "optional with false value");
  EXPECT_FATAL_FAILURE(
      ASSERT_OPTIONAL_FALSE(none), "optional with false value");
}

TEST(optional, expect_optional_true) {
  static std::optional<int> t = 1;
  static std::optional<int> f = 0;
  static std::optional<int> none;
  EXPECT_OPTIONAL_TRUE(t);
  EXPECT_NONFATAL_FAILURE(EXPECT_OPTIONAL_TRUE(f), "optional with true value");
  EXPECT_FATAL_FAILURE(EXPECT_OPTIONAL_TRUE(none), "optional with true value");
}

TEST(optional, expect_optional_false) {
  static std::optional<int> t = 1;
  static std::optional<int> f = 0;
  static std::optional<int> none;
  EXPECT_OPTIONAL_FALSE(f);
  EXPECT_NONFATAL_FAILURE(
      EXPECT_OPTIONAL_FALSE(t), "optional with false value");
  EXPECT_FATAL_FAILURE(
      EXPECT_OPTIONAL_FALSE(none), "optional with false value");
}

TEST(optional, assert_optional_eq_ne) {
  static std::optional<int> v = 5;
  static std::optional<int> none;
  ASSERT_OPTIONAL_EQ(v, 5);
  ASSERT_OPTIONAL_NE(v, 6);
  EXPECT_FATAL_FAILURE(ASSERT_OPTIONAL_EQ(v, 6), "equal to 6");
  EXPECT_FATAL_FAILURE(ASSERT_OPTIONAL_EQ(none, 5), "equal to 5");
  EXPECT_FATAL_FAILURE(ASSERT_OPTIONAL_NE(v, 5), "not equal to 5");
  EXPECT_FATAL_FAILURE(ASSERT_OPTIONAL_NE(none, 5), "not equal to 5");
}

TEST(optional, expect_optional_eq_ne) {
  static std::optional<int> v = 5;
  static std::optional<int> none;
  EXPECT_OPTIONAL_EQ(v, 5);
  EXPECT_OPTIONAL_NE(v, 6);
  EXPECT_NONFATAL_FAILURE(EXPECT_OPTIONAL_EQ(v, 6), "equal to 6");
  EXPECT_FATAL_FAILURE(EXPECT_OPTIONAL_EQ(none, 5), "equal to 5");
  EXPECT_NONFATAL_FAILURE(EXPECT_OPTIONAL_NE(v, 5), "not equal to 5");
  EXPECT_FATAL_FAILURE(EXPECT_OPTIONAL_NE(none, 5), "not equal to 5");
}

TEST(optional, assert_optional_error) {
  static stdext::expected<int, int> ok = 1;
  static stdext::expected<int, int> err = stdext::unexpected<int>(EINVAL);
  ASSERT_OPTIONAL_ERROR(err, EINVAL);
  EXPECT_FATAL_FAILURE(
      ASSERT_OPTIONAL_ERROR(err, ENOTSUP), "Expected: ENOTSUP");
  EXPECT_FATAL_FAILURE(ASSERT_OPTIONAL_ERROR(ok, EINVAL), "no value");
}

TEST(optional, expect_optional_error) {
  static stdext::expected<int, int> ok = 1;
  static stdext::expected<int, int> err = stdext::unexpected<int>(EINVAL);
  EXPECT_OPTIONAL_ERROR(err, EINVAL);
  EXPECT_NONFATAL_FAILURE(
      EXPECT_OPTIONAL_ERROR(err, ENOTSUP), "Expected: ENOTSUP");
  EXPECT_FATAL_FAILURE(EXPECT_OPTIONAL_ERROR(ok, EINVAL), "no value");
}

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
int no_value_calls = 0;

auto no_value_once() -> std::optional<int> {
  ++no_value_calls;
  return std::nullopt;
}

}  // namespace

TEST(optional, assert_optional_has_error) {
  static std::optional<int> v = 5;
  static std::optional<int> none;
  static stdext::expected<int, int> ok = 1;
  static stdext::expected<int, int> err = stdext::unexpected<int>(EINVAL);
  ASSERT_OPTIONAL_HAS_ERROR(none);
  ASSERT_OPTIONAL_HAS_ERROR(err);
  EXPECT_FATAL_FAILURE(ASSERT_OPTIONAL_HAS_ERROR(v), "has value");
  EXPECT_FATAL_FAILURE(ASSERT_OPTIONAL_HAS_ERROR(ok), "has value");
}

TEST(optional, assert_optional_has_error_evaluates_once) {
  no_value_calls = 0;
  ASSERT_OPTIONAL_HAS_ERROR(no_value_once());
  EXPECT_EQ(no_value_calls, 1);
}

namespace {

auto expect_eq_or_return(int expected, int actual) -> bool {
  EXPECT_EQ_OR_RETURN(expected, actual);
  return true;
}

auto expect_ne_or_return(int expected, int actual) -> bool {
  EXPECT_NE_OR_RETURN(expected, actual);
  return true;
}

auto expect_true_or_return(bool condition) -> bool {
  EXPECT_TRUE_OR_RETURN(condition);
  return true;
}

auto expect_false_or_return(bool condition) -> bool {
  EXPECT_FALSE_OR_RETURN(condition);
  return true;
}

auto expect_true_once(int& evaluations) -> bool {
  EXPECT_TRUE_OR_RETURN(++evaluations == 1);
  return true;
}

auto usable_in_unbraced_if(bool condition) -> bool {
  if (condition)
    EXPECT_TRUE_OR_RETURN(condition);
  else
    EXPECT_FALSE_OR_RETURN(condition);
  return true;
}

}  // namespace

TEST(optional, eq_equal_continues) { EXPECT_TRUE(expect_eq_or_return(1, 1)); }

TEST(optional, eq_unequal_fails_and_returns_false) {
  EXPECT_NONFATAL_FAILURE(
      EXPECT_FALSE(expect_eq_or_return(1, 2)), "Expected equality");
}

TEST(optional, ne_unequal_continues) { EXPECT_TRUE(expect_ne_or_return(1, 2)); }

TEST(optional, ne_equal_fails_and_returns_false) {
  EXPECT_NONFATAL_FAILURE(
      EXPECT_FALSE(expect_ne_or_return(1, 1)), "(expected) != (actual)");
}

TEST(optional, true_true_continues) {
  EXPECT_TRUE(expect_true_or_return(true));
}

TEST(optional, true_false_fails_and_returns_false) {
  EXPECT_NONFATAL_FAILURE(
      EXPECT_FALSE(expect_true_or_return(false)), "Value of: condition");
}

TEST(optional, false_false_continues) {
  EXPECT_TRUE(expect_false_or_return(false));
}

TEST(optional, false_true_fails_and_returns_false) {
  EXPECT_NONFATAL_FAILURE(
      EXPECT_FALSE(expect_false_or_return(true)), "Value of: condition");
}

TEST(optional, true_condition_evaluated_once) {
  int evaluations = 0;
  EXPECT_TRUE(expect_true_once(evaluations));
  EXPECT_EQ(evaluations, 1);
}

TEST(optional, usable_as_unbraced_if_body) {
  EXPECT_TRUE(usable_in_unbraced_if(true));
  EXPECT_TRUE(usable_in_unbraced_if(false));
}

TEST(optional, maybe_optional_error_value_present_is_silent) {
  static stdext::expected<int, int> ok = 1;
  ::testing::TestPartResultArray results;
  bool reached_after = false;
  {
    ::testing::ScopedFakeTestPartResultReporter reporter(
        ::testing::ScopedFakeTestPartResultReporter::
            INTERCEPT_ONLY_CURRENT_THREAD,
        &results);
    MAYBE_OPTIONAL_ERROR(ok, EINVAL);
    reached_after = true;
  }
  EXPECT_EQ(results.size(), 0);
  EXPECT_TRUE(reached_after);
}

TEST(optional, maybe_optional_error_value_present_skips_error_check) {
  struct Probe {
    int* calls;
    [[nodiscard]] auto has_value() const -> bool { return true; }
    [[nodiscard]] auto error() const -> int {
      ++*calls;
      return 0;
    }
  };
  int calls = 0;
  Probe probe{&calls};
  MAYBE_OPTIONAL_ERROR(probe, EINVAL);
  EXPECT_EQ(calls, 0);
}

TEST(optional, maybe_optional_error_matching_error_passes) {
  static stdext::expected<int, int> err = stdext::unexpected<int>(EINVAL);
  MAYBE_OPTIONAL_ERROR(err, EINVAL);
}

TEST(optional, maybe_optional_error_mismatched_error_is_nonfatal) {
  static stdext::expected<int, int> err = stdext::unexpected<int>(EINVAL);
  EXPECT_NONFATAL_FAILURE(MAYBE_OPTIONAL_ERROR(err, ENOTSUP),
      "Expected: " + std::to_string(ENOTSUP));
}
