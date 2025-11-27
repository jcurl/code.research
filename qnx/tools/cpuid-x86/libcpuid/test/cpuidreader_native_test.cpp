#include "cpuidreader_native.h"

#include <set>
#include <thread>

#include <gtest/gtest.h>

#include "cpuid/cpuid.h"

TEST(cpuidreader_native, has_cpuid) {
  rjcp::cpuid::cpuidreader_native cpu{};

  ASSERT_TRUE(cpu.has_cpuid());
}

TEST(cpuidreader_native, cpuid_zero) {
  rjcp::cpuid::cpuidreader_native cpu{};

  // Check the first register, whose result is expected to be not more than
  // 0xFF leaves.
  auto reg = cpu.cpuid(0, 0);
  ASSERT_TRUE(reg.has_value());
  if (reg) {
    ASSERT_EQ(reg->eax & 0xFFFFFF00, 0);
  }
}

TEST(cpuidreader_native, cpuid_ext) {
  rjcp::cpuid::cpuidreader_native cpu{};

  // Check the extended register, whose result is expected to be not more than
  // 0xFF leaves.
  auto reg = cpu.cpuid(0x80000000, 0);
  ASSERT_TRUE(reg.has_value());
  if (reg) {
    ASSERT_EQ(reg->eax & 0xFFFFFF00, 0x80000000);
  }
}

TEST(cpuidreader_native, cpuid_threads) {
  rjcp::cpuid::cpuidreader_native cpu{};

  std::set<rjcp::cpuid::cpuidreg> acpi{};
  for (unsigned int core = 0; core < std::thread::hardware_concurrency();
       core++) {
    auto pin = cpu.enable_core(core);
    EXPECT_TRUE(pin.has_value());

    auto result = cpu.cpuid(0x00000001, 0x00000000);
    EXPECT_TRUE(result.has_value());
    if (result.has_value()) {
      // NOTE: This test can fail if run from within valgrind, which virtualises
      // the CPUID instruction. Using the /dev/cpu/N/cpuid will still work!
      EXPECT_TRUE(acpi.find(result->ebx) == acpi.end());
      acpi.insert(result->ebx);
    }
  }
}
