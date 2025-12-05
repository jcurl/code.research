#include "cpuid/cpuidreader_native.h"

#include <set>

#include <gtest/gtest.h>

#include "ubench/string.h"

using namespace rjcp::cpuid;

// Needed so that clang-tidy doesn't complain about values being used without
// checking the condition first.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ASSERT_HAS_VALUE(variable)       \
  {                                      \
    ASSERT_TRUE((variable).has_value()); \
    if (!(variable).has_value()) return; \
  }

TEST(cpuidreader_native, has_cpuid) {
  cpuidreader_native cpu{};

  ASSERT_TRUE(cpu.has_cpuid());
}

TEST(cpuidreader_native, make_cpuidreader) {
  auto cpu = make_cpuidreader<cpuidreader_native>();

  ASSERT_TRUE(cpu->has_cpuid());
}

TEST(cpuidreader_native, cpuid_zero) {
  cpuidreader_native cpu{};

  // Check the first register, whose result is expected to be not more than
  // 0xFF leaves.
  auto reg = cpu.cpuid(0, 0);
  ASSERT_HAS_VALUE(reg);
  ASSERT_EQ(reg->eax & 0xFFFFFF00, 0);
}

TEST(cpuidreader_native, cpuid_ext) {
  cpuidreader_native cpu{};

  // Check the extended register, whose result is expected to be not more than
  // 0xFF leaves.
  auto reg = cpu.cpuid(0x80000000, 0);
  ASSERT_HAS_VALUE(reg);
  ASSERT_EQ(reg->eax & 0xFFFFFF00, 0x80000000);
}

TEST(cpuidreader_native, cpuid_threads) {
  cpuidreader_native cpu{};

  std::set<cpuidreg> acpi{};
  for (unsigned int core = 0; core < cpu.cores(); core++) {
    auto pin = cpu.enable_core(core);
    ASSERT_TRUE(pin) << ubench::string::perror(pin.error());
    ASSERT_TRUE(*pin);
    EXPECT_EQ((*pin)->core(), core);

    auto result = cpu.cpuid(0x00000001, 0x00000000);
    ASSERT_HAS_VALUE(result);

    // NOTE: This test can fail if run from within valgrind, which virtualises
    // the CPUID instruction. Using the /dev/cpu/N/cpuid will still work!
    EXPECT_TRUE(acpi.find(result->ebx) == acpi.end());
    acpi.insert(result->ebx);
  }
}
