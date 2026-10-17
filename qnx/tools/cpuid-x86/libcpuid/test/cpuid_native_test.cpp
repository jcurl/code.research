#include "cpuid/cpuid_native.h"

#include <gtest/gtest.h>

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

TEST(cpuid_native, has_cpuid) {
  cpuid_native cpu{};

  ASSERT_EQ(cpu.has_cpuid(), has_cpuid_driver());
}

TEST(cpuid_native, cpuid_zero) {
  cpuid_native cpu{};
  if (!has_cpuid_driver()) GTEST_SKIP() << "Not on X86 hardware";

  auto reg = cpu.cpuid(0, 0);

  // Check the first register, whose result is expected to be not more than
  // 0xFF leaves.
  ASSERT_OPTIONAL(reg);
  ASSERT_EQ(reg->eax & 0xFFFFFF00, 0);
}

TEST(cpuid_native, cpuid_ext) {
  cpuid_native cpu{};
  if (!has_cpuid_driver()) GTEST_SKIP() << "Not on X86 hardware";

  auto reg = cpu.cpuid(0x80000000, 0);

  // Check the extended register, whose result is expected to be not more than
  // 0xFF leaves.
  ASSERT_OPTIONAL(reg);
  ASSERT_EQ(reg->eax & 0xFFFFFF00, 0x80000000);
}
