#include "cpuid/cpuid_dev.h"

#include <gtest/gtest.h>

#include "ubench/file.h"
#include "cpuid_dev.h"

using namespace rjcp::cpuid;

// Needed so that clang-tidy doesn't complain about values being used without
// checking the condition first.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ASSERT_HAS_VALUE(variable)       \
  {                                      \
    ASSERT_TRUE((variable).has_value()); \
    if (!(variable).has_value()) return; \
  }

TEST(cpuid_dev, has_cpuid_invalid) {
  cpuid_dev cpu{-1};
  ASSERT_FALSE(cpu.has_cpuid());
}

TEST(cpuid_dev, has_cpuid) {
  auto fd = get_cpuid_dev_fdpath(0);
  if (!fd) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  cpuid_dev cpu{fd};
  ASSERT_TRUE(cpu.has_cpuid());
  close(fd);
}

TEST(cpuid_dev, cpuid_zero) {
  auto fd = get_cpuid_dev_fdpath(0);
  if (!fd) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  cpuid_dev cpu{fd};
  auto reg = cpu.cpuid(0, 0);

  // Check the first register, whose result is expected to be not more than
  // 0xFF leaves.
  ASSERT_HAS_VALUE(reg);
  ASSERT_EQ(reg->eax & 0xFFFFFF00, 0);
}

TEST(cpuid_dev, cpuid_ext) {
  auto fd = get_cpuid_dev_fdpath(0);
  if (!fd) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  cpuid_dev cpu{fd};
  auto reg = cpu.cpuid(0x80000000, 0);

  // Check the extended register, whose result is expected to be not more than
  // 0xFF leaves.
  ASSERT_HAS_VALUE(reg);
  ASSERT_EQ(reg->eax & 0xFFFFFF00, 0x80000000);
}

TEST(cpuid_dev, move_ctor) {
  auto fd = get_cpuid_dev_fdpath(0);
  if (!fd) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  cpuid_dev cpu{fd};
  ASSERT_TRUE(cpu.has_cpuid());

  cpuid_dev cpu2{std::move(cpu)};

  // We've moved, but after the move, we expect the object to at least indicate
  // it is no longer valid. This is OK, because the classes are internal only to
  // this implementation (and not public). It helps to show that when the
  // objects that are moved out are destroyed, we have the correct behaviour.

  // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
  EXPECT_FALSE(cpu.has_cpuid());
  EXPECT_TRUE(cpu2.has_cpuid());
}

TEST(cpuid_dev, move_assignment) {
  auto fd = get_cpuid_dev_fdpath(0);
  if (!fd) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  cpuid_dev cpu{fd};
  ASSERT_TRUE(cpu.has_cpuid());

  cpuid_dev cpu2;
  cpu2 = std::move(cpu);

  // We've moved, but after the move, we expect the object to at least indicate
  // it is no longer valid. This is OK, because the classes are internal only to
  // this implementation (and not public). It helps to show that when the
  // objects that are moved out are destroyed, we have the correct behaviour.

  // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
  EXPECT_FALSE(cpu.has_cpuid());
  EXPECT_TRUE(cpu2.has_cpuid());
}
