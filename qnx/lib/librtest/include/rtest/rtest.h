#ifndef LIB_RTEST_RTEST_H
#define LIB_RTEST_RTEST_H

#include <gtest/gtest.h>

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

// For internal usage only. A missing value is always a fatal failure. If the
// value is present but `condition` (which may use *(optional)) is false, the
// failure is reported with `on_mismatch`.
//
// A message streamed with `<<` after the macro is attached only to the
// missing-value failure; a wrong value reports only the standard message.
#define RTEST_OPTIONAL_CHECK_(optional, condition, on_mismatch, message) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_                                          \
  if ((optional).has_value()) {                                          \
    if (condition)                                                       \
      ;                                                                  \
    else                                                                 \
      on_mismatch("Value of: " #optional "\n  Expected: " message);      \
  } else                                                                 \
    GTEST_FATAL_FAILURE_("Value of: " #optional "\n  Expected: " message)

#define ASSERT_OPTIONAL(variable)                \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_                  \
  if (static_cast<bool>((variable).has_value())) \
    ;                                            \
  else                                           \
    GTEST_FATAL_FAILURE_("Value of: " #variable "\n  Expected: true")

// The optional must have a value, and that value must be true / false. For
// EXPECT_*, only a wrong value is non-fatal; a missing value is still fatal, so
// use ASSERT_OPTIONAL first when the value may be missing.
//
// A message streamed after these macros (and the _EQ / _NE macros below) is
// printed only when the value is missing, so it is safe to stream the error:
//   ASSERT_OPTIONAL_TRUE(r) << perror(r.error());
// When the value is present but wrong, the streamed expression is not
// evaluated and only the standard message is reported.
#define ASSERT_OPTIONAL_TRUE(optional)                            \
  RTEST_OPTIONAL_CHECK_(optional, static_cast<bool>(*(optional)), \
      GTEST_FATAL_FAILURE_, "optional with true value")
#define ASSERT_OPTIONAL_FALSE(optional)                            \
  RTEST_OPTIONAL_CHECK_(optional, !static_cast<bool>(*(optional)), \
      GTEST_FATAL_FAILURE_, "optional with false value")
#define EXPECT_OPTIONAL_TRUE(optional)                            \
  RTEST_OPTIONAL_CHECK_(optional, static_cast<bool>(*(optional)), \
      GTEST_NONFATAL_FAILURE_, "optional with true value")
#define EXPECT_OPTIONAL_FALSE(optional)                            \
  RTEST_OPTIONAL_CHECK_(optional, !static_cast<bool>(*(optional)), \
      GTEST_NONFATAL_FAILURE_, "optional with false value")

// The optional must have a value, and that value must be equal / not equal to
// `expected`.
#define ASSERT_OPTIONAL_EQ(optional, expected)               \
  RTEST_OPTIONAL_CHECK_(optional, *(optional) == (expected), \
      GTEST_FATAL_FAILURE_, "optional with value equal to " #expected)
#define ASSERT_OPTIONAL_NE(optional, expected)               \
  RTEST_OPTIONAL_CHECK_(optional, *(optional) != (expected), \
      GTEST_FATAL_FAILURE_, "optional with value not equal to " #expected)
#define EXPECT_OPTIONAL_EQ(optional, expected)               \
  RTEST_OPTIONAL_CHECK_(optional, *(optional) == (expected), \
      GTEST_NONFATAL_FAILURE_, "optional with value equal to " #expected)
#define EXPECT_OPTIONAL_NE(optional, expected)               \
  RTEST_OPTIONAL_CHECK_(optional, *(optional) != (expected), \
      GTEST_NONFATAL_FAILURE_, "optional with value not equal to " #expected)

// For internal usage only. The value must be missing, and error() must equal
// `error_code`. Finding a value is always a fatal failure, since error() is
// then not meaningful. error() is only called when there is no value. A wrong
// error is reported with `on_mismatch`. Not usable with std::optional, which
// has no error().
#define RTEST_OPTIONAL_ERROR_CHECK_(                                    \
    optional, error_code, error_name, on_mismatch)                      \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_                                         \
  if ((optional).has_value()) {                                         \
    GTEST_FATAL_FAILURE_("Value of: " #optional                         \
                         "\n  Actual: has value\n  Expected: no value " \
                         "with error " error_name);                     \
  } else if ((optional).error() == (error_code))                        \
    ;                                                                   \
  else                                                                  \
    on_mismatch("Value of: " #optional ".error()\n  Expected: " error_name)

// ASSERT_OPTIONAL_HAS_ERROR(optional) requires the value to be missing; it
// works with any optional type.
#define ASSERT_OPTIONAL_HAS_ERROR(optional)       \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_                   \
  if (!static_cast<bool>((optional).has_value())) \
    ;                                             \
  else                                            \
    GTEST_FATAL_FAILURE_(                         \
        "Value of: " #optional "\n  Actual: has value\n  Expected: no value")

// For stdext::expected: the value must be missing, and error() must equal
// `error_code`. A wrong error is non-fatal; a present value is fatal.
#define ASSERT_OPTIONAL_ERROR(optional, error_code) \
  RTEST_OPTIONAL_ERROR_CHECK_(                      \
      optional, error_code, #error_code, GTEST_FATAL_FAILURE_)
#define EXPECT_OPTIONAL_ERROR(optional, error_code) \
  RTEST_OPTIONAL_ERROR_CHECK_(                      \
      optional, error_code, #error_code, GTEST_NONFATAL_FAILURE_)

// Like EXPECT_OPTIONAL_ERROR, for calls that may legitimately succeed. A
// present value is accepted silently: nothing is checked, nothing is reported
// and it is never a failure. A missing value must have error() equal to
// `error_code`.
#define MAYBE_OPTIONAL_ERROR(optional, error_code) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_                    \
  if ((optional).has_value())                      \
    ;                                              \
  else                                             \
    EXPECT_OPTIONAL_ERROR(optional, error_code)

// Like EXPECT_*, but also returns false from the enclosing function on failure.
// Only usable in functions returning bool. Arguments are evaluated more than
// once, except for the condition of the _TRUE/_FALSE variants.
#define EXPECT_EQ_OR_RETURN(expected, actual)    \
  do {                                           \
    EXPECT_EQ(expected, actual);                 \
    if (!((expected) == (actual))) return false; \
  } while (false)

#define EXPECT_NE_OR_RETURN(expected, actual)    \
  do {                                           \
    EXPECT_NE(expected, actual);                 \
    if (!((expected) != (actual))) return false; \
  } while (false)

#define EXPECT_TRUE_OR_RETURN(condition)                    \
  do {                                                      \
    if (!static_cast<bool>(condition)) {                    \
      ADD_FAILURE() << "Value of: " #condition              \
                    << "\n  Actual: false\nExpected: true"; \
      return false;                                         \
    }                                                       \
  } while (false)

#define EXPECT_FALSE_OR_RETURN(condition)                   \
  do {                                                      \
    if (static_cast<bool>(condition)) {                     \
      ADD_FAILURE() << "Value of: " #condition              \
                    << "\n  Actual: true\nExpected: false"; \
      return false;                                         \
    }                                                       \
  } while (false)

// NOLINTEND(cppcoreguidelines-macro-usage)

#endif
