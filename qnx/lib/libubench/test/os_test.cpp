#include "ubench/os.h"

#include <iostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ubench/string.h"

using ::testing::Gt;

TEST(get_proc_name, check_not_empty) {
  // If this test fails, then it is not supported on the platform being
  // compiled. Work must be done to port it.
  pid_t self = getpid();
  auto procname = ubench::os::get_proc_name(self);
  EXPECT_TRUE(procname) << "error " << ubench::string::perror(procname.error());
  if (procname) {
    ASSERT_THAT(procname->length(), Gt(0));
    std::cout << *procname << std::endl;
  }
}
