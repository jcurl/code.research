#include "ubench/clock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Eq;
using ::testing::Ne;

TEST(idle_clock, is_available) {
  // If this test fails, then it is not supported on the platform being
  // compiled. Work must be done to port it.
  ASSERT_THAT(ubench::chrono::idle_clock::is_available(), Eq(true));
  ASSERT_THAT(ubench::chrono::idle_clock::type(),
      Ne(ubench::chrono::idle_clock_type::null));
}

TEST(process_clock, is_available) {
  // If this test fails, then it is not supported on the platform being
  // compiled. Work must be done to port it.
  ASSERT_THAT(ubench::chrono::process_clock::is_available(), Eq(true));
}
