#include "cpuid_native.h"

#include <gtest/gtest.h>

TEST(cpuid_native, has_cpuid) {
  rjcp::cpuid::cpuid_native cpu{};

  ASSERT_TRUE(cpu.has_cpuid());
}

TEST(cpuid_native, cpuid_zero) {
  rjcp::cpuid::cpuid_native cpu{};
  auto reg = cpu.cpuid(0, 0);

  // Check the first register, whose result is expected to be not more than
  // 0xFF leaves.
  ASSERT_TRUE(reg.has_value());
  if (reg) {
    ASSERT_EQ(reg->eax & 0xFFFFFF00, 0);
  }
}

TEST(cpuid_native, cpuid_ext) {
  rjcp::cpuid::cpuid_native cpu{};
  auto reg = cpu.cpuid(0x80000000, 0);

  // Check the extended register, whose result is expected to be not more than
  // 0xFF leaves.
  ASSERT_TRUE(reg.has_value());
  if (reg) {
    ASSERT_EQ(reg->eax & 0xFFFFFF00, 0x80000000);
  }
}
