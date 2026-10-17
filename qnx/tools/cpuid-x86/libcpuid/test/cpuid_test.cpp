#include "cpuid/cpuid.h"

#include <iomanip>
#include <ios>
#include <iostream>

#include <gtest/gtest.h>

#include "cpuid/cpuidreader_native.h"
#include "rtest/rtest.h"

using namespace rjcp::cpuid;

namespace {

auto has_cpuid_driver() -> bool {
#if __x86_64__ || __amd64__ || __i386__
  return true;
#else
  return false;
#endif
}

}  // namespace

TEST(cpuid, dump) {
  if (!has_cpuid_driver()) GTEST_SKIP() << "Not on X86 hardware";

  auto reader = make_cpuidreader<cpuidreader_native>();
  ASSERT_TRUE(reader);

  auto dump = cpuid_dump(*reader, 0);
  ASSERT_OPTIONAL(dump);

  for (auto reg : *dump) {
    std::cout << std::hex << std::setfill('0') << std::setw(8) << reg.req.eax
              << " " << std::setfill('0') << std::setw(8) << reg.req.ecx << ": "
              << std::setfill('0') << std::setw(8) << reg.res.eax << " "
              << std::setfill('0') << std::setw(8) << reg.res.ebx << " "
              << std::setfill('0') << std::setw(8) << reg.res.ecx << " "
              << std::setfill('0') << std::setw(8) << reg.res.edx << std::endl;
  }
}