#include "cpuid/cpuid_cache.h"

#include <gtest/gtest.h>

using namespace rjcp::cpuid;

// Needed so that clang-tidy doesn't complain about values being used without
// checking the condition first.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ASSERT_HAS_VALUE(variable)       \
  {                                      \
    ASSERT_TRUE((variable).has_value()); \
    if (!(variable).has_value()) return; \
  }

const std::vector<cpuid_info> default_data{
    {{0, 0}, {0, 0x56e6547, 0x6c65746e, 0x49656e69}},
    {{0x80000000, 0}, {0x80000000, 0, 0, 0}},
};

TEST(cpuid_cache, has_cpuid_nocache) {
  cpuid_cache cpu{};

  ASSERT_FALSE(cpu.has_cpuid());
}

TEST(cpuid_cache, has_cpuid_cache) {
  cpuid_cache cpu{default_data};

  ASSERT_TRUE(cpu.has_cpuid());
}

TEST(cpuid_cache, cpuid_zero) {
  cpuid_cache cpu{default_data};
  auto reg = cpu.cpuid(0, 0);

  // Check the first register, whose result is expected to be not more than
  // 0xFF leaves.
  ASSERT_HAS_VALUE(reg);
  ASSERT_EQ(reg->eax, 0);
}

TEST(cpuid_cache, cpuid_ext) {
  cpuid_cache cpu{default_data};
  auto reg = cpu.cpuid(0x80000000, 0);

  // Check the extended register, whose result is expected to be not more than
  // 0xFF leaves.
  ASSERT_HAS_VALUE(reg);
  ASSERT_EQ(reg->eax, 0x80000000);
}

TEST(cpuid_cache, cpuid_updatecache) {
  cpuid_cache cpu(default_data);

  cpuid_info info0{{0, 0}, {0x1, 0x756e6547, 0x6c65746e, 0x49656e69}};
  cpuid_info info1{{1, 0}, {0xb0671, 0x800800, 0x7ffafbbf, 0xbfebfbff}};
  ASSERT_FALSE(cpu.update_cache(info0));
  ASSERT_TRUE(cpu.update_cache(info1));

  auto reg0 = cpu.cpuid(0, 0);
  ASSERT_HAS_VALUE(reg0);
  ASSERT_EQ(reg0->eax, 0x1);

  auto reg1 = cpu.cpuid(1, 0);
  ASSERT_HAS_VALUE(reg1);
  ASSERT_EQ(reg1->eax, 0xb0671);
}

TEST(cpuid_cache, cpuid_empty_updatecache) {
  cpuid_cache cpu{};

  ASSERT_FALSE(cpu.has_cpuid());

  cpuid_info info0{{0, 0}, {0x1, 0x756e6547, 0x6c65746e, 0x49656e69}};
  ASSERT_TRUE(cpu.update_cache(info0));
  ASSERT_TRUE(cpu.has_cpuid());

  auto reg0 = cpu.cpuid(0, 0);
  ASSERT_HAS_VALUE(reg0);
  ASSERT_EQ(reg0->eax, 0x1);

  auto reg1 = cpu.cpuid(1, 0);
  ASSERT_FALSE(reg1.has_value());
}
