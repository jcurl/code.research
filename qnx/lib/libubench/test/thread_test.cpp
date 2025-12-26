#include "ubench/thread.h"

#include <thread>

#include <gtest/gtest.h>

#include "ubench/string.h"

TEST(pin_core, pin_core_zero) {
  auto pin = ubench::thread::pin_core(0);
  EXPECT_TRUE(pin) << "Failed to pin to core 0: "
                   << ubench::string::perror(pin.error());
  EXPECT_EQ(pin.core(), 0);
}

TEST(pin_core, pin_core_large) {
  auto pin = ubench::thread::pin_core(1024);
  EXPECT_FALSE(pin);
}

TEST(thread_count, get_count) {
  auto count_default = ubench::thread::thread_count();
  EXPECT_NE(count_default, 0);

  auto hw_default = std::thread::hardware_concurrency();
  ASSERT_NE(hw_default, 0);

  auto pin = ubench::thread::pin_core(0);
  auto count_pinned = ubench::thread::thread_count();
  EXPECT_NE(count_pinned, 0);

  auto hw_pinned = std::thread::hardware_concurrency();
  ASSERT_NE(hw_pinned, 0);

  EXPECT_EQ(count_pinned, count_default);
  if (hw_default != hw_pinned) {
    std::cout << "Warning: std::thread::hardware_concurrency() changed from "
              << hw_default << " to " << hw_pinned << " when pinning."
              << std::endl;
  }
}
