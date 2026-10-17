#include "cpuid/cpuidreader_cpuctl.h"

#include <filesystem>
#include <set>

#include <gtest/gtest.h>

#include "rtest/rtest.h"
#include "ubench/string.h"

using namespace rjcp::cpuid;

namespace {

auto has_cpuid_driver() -> bool {
  std::string path = std::string{"/dev/cpuctl0"};
  std::filesystem::path dev{path};

  return std::filesystem::exists(dev);
}

}  // namespace

TEST(cpuidreader_cpuctl, has_cpuid) {
  cpuidreader_cpuctl cpu{};

  ASSERT_EQ(cpu.has_cpuid(), has_cpuid_driver());
}

TEST(cpuidreader_cpuctl, make_cpuidreader) {
  auto cpu = make_cpuidreader<cpuidreader_cpuctl>();

  ASSERT_EQ(cpu->has_cpuid(), has_cpuid_driver());
}

TEST(cpuidreader_cpuctl, cpuid_zero) {
  cpuidreader_cpuctl cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  // Check the first register, whose result is expected to be not more than
  // 0xFF leaves.
  auto reg = cpu.cpuid(0, 0);
  ASSERT_OPTIONAL(reg);
  ASSERT_EQ(reg->eax & 0xFFFFFF00, 0);
}

TEST(cpuidreader_cpuctl, cpuid_ext) {
  cpuidreader_cpuctl cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  // Check the extended register, whose result is expected to be not more than
  // 0xFF leaves.
  auto reg = cpu.cpuid(0x80000000, 0);
  ASSERT_OPTIONAL(reg);
  ASSERT_EQ(reg->eax & 0xFFFFFF00, 0x80000000);
}

TEST(cpuidreader_cpuctl, cpuid_context_move) {
  cpuidreader_cpuctl cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  unsigned int core = 0;
  if (cpu.cores() > 1) core = 1;

  auto pin = cpu.enable_core(core);
  ASSERT_OPTIONAL_TRUE(pin) << ubench::string::perror(pin.error());

  auto reg0 = cpu.cpuid(1, 0);
  ASSERT_OPTIONAL(reg0);
  auto reg0ebx = reg0->ebx;

  decltype(pin) pin2 = std::move(pin);
  ASSERT_OPTIONAL_TRUE(pin2) << ubench::string::perror(pin2.error());

  auto reg1 = cpu.cpuid(1, 0);
  ASSERT_OPTIONAL(reg1);
  auto reg1ebx = reg1->ebx;

  // Moving the context should not affect which core is being read. We check
  // that the cores are the same by comparing the actual CPUID.01 EBX which is
  // unique per core.
  ASSERT_EQ(reg0ebx, reg1ebx);
}

TEST(cpuidreader_cpuctl, cpuid_threads) {
  cpuidreader_cpuctl cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  std::set<cpuidreg> acpi{};
  for (unsigned int core = 0; core < cpu.cores(); core++) {
    auto pin = cpu.enable_core(core);
    ASSERT_OPTIONAL_TRUE(pin) << ubench::string::perror(pin.error());

    auto result = cpu.cpuid(0x00000001, 0x00000000);
    ASSERT_OPTIONAL(result);
    EXPECT_TRUE(acpi.find(result->ebx) == acpi.end());
    acpi.insert(result->ebx);
  }
}

TEST(cpuidreader_cpuctl, cpuid_threads_context_move) {
  cpuidreader_cpuctl cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  std::set<cpuidreg> acpi{};
  for (unsigned int core = 0; core < cpu.cores(); core++) {
    auto pin = cpu.enable_core(core);
    ASSERT_OPTIONAL_TRUE(pin) << ubench::string::perror(pin.error());

    auto pin2 = std::move(pin);
    ASSERT_OPTIONAL_TRUE(pin2) << ubench::string::perror(pin2.error());

    auto result = cpu.cpuid(0x00000001, 0x00000000);
    ASSERT_OPTIONAL(result);
    EXPECT_TRUE(acpi.find(result->ebx) == acpi.end());
    acpi.insert(result->ebx);
  }
}

TEST(cpuidreader_cpuctl, cpuid_query_then_enable) {
  cpuidreader_cpuctl cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  // Query using the default core (0).
  auto regd = cpu.cpuid(1, 0);
  ASSERT_OPTIONAL(regd);
  auto regdebx = regd->ebx;

  // Now enable a specific core, which should succeed.
  unsigned int core = 0;
  if (cpu.cores() > 1) core = 1;

  auto pin = cpu.enable_core(core);
  ASSERT_OPTIONAL_TRUE(pin) << ubench::string::perror(pin.error());
  EXPECT_EQ((*pin)->core(), core);

  auto regc = cpu.cpuid(1, 0);
  ASSERT_OPTIONAL(regc);
  auto regcebx = regc->ebx;

  // They should be different, unless we queried the same core.
  ASSERT_EQ(regcebx == regdebx, core == 0);
}

TEST(cpuidreader_cpuctl, cpuid_enable_core_twice_fail) {
  cpuidreader_cpuctl cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  unsigned int core = 0;
  if (cpu.cores() > 1) core = 1;

  auto pin = cpu.enable_core(core);
  ASSERT_OPTIONAL_TRUE(pin) << ubench::string::perror(pin.error());
  EXPECT_EQ((*pin)->core(), core);

  // The second activation should fail, because the first activation is still
  // valid.

  auto pin2 = cpu.enable_core(core);
  ASSERT_FALSE(pin2);
  ASSERT_EQ(pin2.error(), EINVAL);
}

TEST(cpuidreader_cpuctl, cpuid_enable_core_twice_success) {
  cpuidreader_cpuctl cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  unsigned int core = 0;
  if (cpu.cores() > 1) core = 1;

  cpuidreg regc = 0;
  {
    auto pin = cpu.enable_core(core);
    ASSERT_OPTIONAL_TRUE(pin) << ubench::string::perror(pin.error());
    EXPECT_EQ((*pin)->core(), core);
    auto reg = cpu.cpuid(1, 0);
    ASSERT_OPTIONAL(reg);
    regc = reg->ebx;
  }

  // The second activation should succeed because the first activation went out
  // of scope.
  cpuidreg reg0 = 0;
  {
    auto pin = cpu.enable_core(0);
    ASSERT_OPTIONAL_TRUE(pin) << ubench::string::perror(pin.error());
    EXPECT_EQ((*pin)->core(), 0);
    auto reg = cpu.cpuid(1, 0);
    ASSERT_OPTIONAL(reg);
    reg0 = reg->ebx;
  }

  ASSERT_EQ(regc == reg0, core == 0);
}

TEST(cpuidreader_cpuctl, move_ctor) {
  cpuidreader_cpuctl cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  unsigned int core = 0;
  if (cpu.cores() > 1) core = 1;

  cpuidreg regebx{}, regmebx{};
  {
    auto pin = cpu.enable_core(core);
    ASSERT_OPTIONAL_TRUE(pin) << ubench::string::perror(pin.error());
    EXPECT_EQ((*pin)->core(), core);
    auto reg = cpu.cpuid(1, 0);
    ASSERT_OPTIONAL(reg);
    regebx = reg->ebx;

    // Move and test we get the same results.

    decltype(cpu) cpu2{std::move(cpu)};
    auto regm = cpu2.cpuid(1, 0);
    ASSERT_OPTIONAL(regm);
    regmebx = regm->ebx;
  }

  ASSERT_EQ(regebx, regmebx);
}

TEST(cpuidreader_cpuctl, move_assignment) {
  cpuidreader_cpuctl cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  cpuidreader_cpuctl cpu2{};

  unsigned int core = 0;
  if (cpu.cores() > 1) core = 1;

  cpuidreg regebx{}, regmebx{};
  {
    auto pin = cpu.enable_core(core);
    ASSERT_OPTIONAL_TRUE(pin) << ubench::string::perror(pin.error());
    EXPECT_EQ((*pin)->core(), core);
    auto reg = cpu.cpuid(1, 0);
    ASSERT_OPTIONAL(reg);
    regebx = reg->ebx;

    // Move and test we get the same results.

    cpu2 = std::move(cpu);
    auto regm = cpu2.cpuid(1, 0);
    ASSERT_OPTIONAL(regm);
    regmebx = regm->ebx;
  }

  ASSERT_EQ(regebx, regmebx);

  auto pin2 = cpu2.enable_core(0);
  ASSERT_OPTIONAL_TRUE(pin2) << ubench::string::perror(pin2.error());
}

TEST(cpuidreader_cpuctl, oob_cores) {
  cpuidreader_cpuctl cpu{};

  auto pin = cpu.enable_core(ubench::thread::thread_count());
  ASSERT_FALSE(pin.has_value());
  ASSERT_NE(pin.error(), 0);
}
