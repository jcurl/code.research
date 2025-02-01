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

  auto time = ubench::chrono::idle_clock::now();
  EXPECT_THAT(time.time_since_epoch().count(), Ne(0));
}

TEST(process_clock, is_available) {
  // If this test fails, then it is not supported on the platform being
  // compiled. Work must be done to port it.
  ASSERT_THAT(ubench::chrono::process_clock::is_available(), Eq(true));

  // We want to spin for some time to allow the clock to at least increment
  // by one tick, so that when we read it, the result is not zero.
  volatile int cycles = 1000000000;
  while (ubench::chrono::process_clock::now().time_since_epoch().count() == 0 &&
         cycles > 0) {
    --cycles;
  }
  auto time = ubench::chrono::process_clock::now();
  EXPECT_THAT(time.time_since_epoch().count(), Ne(0));
}
