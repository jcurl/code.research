#include "ubench/os.h"

#include <iostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Gt;

TEST(get_proc_name, check_not_empty) {
  pid_t self = getpid();
  auto procname = ubench::os::get_proc_name(self);
  EXPECT_TRUE(procname);
  if (procname) {
    EXPECT_THAT(procname->length(), Gt(0));
    std::cout << *procname << std::endl;
  }
}
