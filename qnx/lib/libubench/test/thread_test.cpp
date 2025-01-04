#include "ubench/thread.h"

#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Eq;

TEST(pin_core, pin_core_no_error) {
  // If this test fails, then it is not supported on the platform being
  // compiled. Work must be done to port it.
  int success = 0;

  std::thread thread_pin([&success]() {
    if (!ubench::thread::pin_core(0)) {
      success = errno;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  });
  thread_pin.join();
  ASSERT_THAT(success, 0);
}
