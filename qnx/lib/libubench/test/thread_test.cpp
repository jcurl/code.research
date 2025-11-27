#include "ubench/thread.h"

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
