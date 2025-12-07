#include "cpuid/cpuid.h"

#include <algorithm>
#include <iomanip>
#include <ios>
#include <iostream>

#include <gtest/gtest.h>

#include "cpuid/cpuidreader_native.h"

using namespace rjcp::cpuid;

// Needed so that clang-tidy doesn't complain about values being used without
// checking the condition first.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ASSERT_HAS_VALUE(variable)       \
  {                                      \
    ASSERT_TRUE((variable).has_value()); \
    if (!(variable).has_value()) return; \
  }

TEST(cpuid, dump) {
  auto reader = make_cpuidreader<cpuidreader_native>();
  ASSERT_TRUE(reader);

  auto dump = cpuid_dump(*reader, 0);
  ASSERT_HAS_VALUE(dump);

  for (auto reg : *dump) {
    std::cout << std::hex << std::setfill('0') << std::setw(8) << reg.req.eax
              << " " << std::setfill('0') << std::setw(8) << reg.req.ecx << ": "
              << std::setfill('0') << std::setw(8) << reg.res.eax << " "
              << std::setfill('0') << std::setw(8) << reg.res.ebx << " "
              << std::setfill('0') << std::setw(8) << reg.res.ecx << " "
              << std::setfill('0') << std::setw(8) << reg.res.edx << std::endl;
  }
}