#include "osqnx/asinfo.h"

#include <iomanip>
#include <iostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(asinfo, is_ordered) {
  auto asinfo = os::qnx::get_asinfo();
  EXPECT_NE(asinfo.size(), 0);

  uintptr_t last = 0;
  for (const auto& entry : asinfo) {
    EXPECT_GE(entry.start, last);
    last = entry.start;
  }
}

TEST(asinfo, dump) {
  auto asinfo = os::qnx::get_asinfo();
  EXPECT_NE(asinfo.size(), 0);

  std::cout << std::hex;
  for (const auto& entry : asinfo) {
    std::cout << std::setfill('0') << std::setw(16) << entry.start << " - "
              << std::setfill('0') << std::setw(16) << entry.end << ": "
              << entry.name << std::endl;
  }
  std::cout << std::dec;
}

TEST(asinfo, in_range) {
  auto asinfo = os::qnx::get_asinfo();
  EXPECT_NE(asinfo.size(), 0);

  for (const auto& entry : asinfo) {
    if (entry.size() > 0) {
      EXPECT_TRUE(entry.in_range(entry.start));
      EXPECT_TRUE(entry.in_range(entry.end));
    }
  }
}

TEST(asinfo, sysram_size) {
  auto size = os::qnx::get_sysram();
  std::cout << "Total Memory (bytes): " << std::hex << size << std::dec
            << std::endl;
  ;
  EXPECT_NE(size, 0);
}
