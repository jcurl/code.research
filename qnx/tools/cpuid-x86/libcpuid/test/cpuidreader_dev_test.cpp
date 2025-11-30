#include "cpuidreader_dev.h"

#include <filesystem>
#include <set>
#include <thread>

#include <gtest/gtest.h>

#include "cpuid/cpuid.h"
#include "ubench/string.h"

// Needed so that clang-tidy doesn't complain about values being used without
// checking the condition first.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ASSERT_HAS_VALUE(variable)       \
  {                                      \
    ASSERT_TRUE((variable).has_value()); \
    if (!(variable).has_value()) return; \
  }

TEST(cpuidreader_dev, has_cpuid) {
  rjcp::cpuid::cpuidreader_dev cpu{};

  std::string path = std::string{"/dev/cpu/0/cpuid"};
  std::filesystem::path dev{path};

  bool present = std::filesystem::exists(dev);
  ASSERT_EQ(cpu.has_cpuid(), present);
}

TEST(cpuidreader_dev, cpuid_zero) {
  rjcp::cpuid::cpuidreader_dev cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  // Check the first register, whose result is expected to be not more than
  // 0xFF leaves.
  auto reg = cpu.cpuid(0, 0);
  ASSERT_HAS_VALUE(reg);
  ASSERT_EQ(reg->eax & 0xFFFFFF00, 0);
}

TEST(cpuidreader_dev, cpuid_ext) {
  rjcp::cpuid::cpuidreader_dev cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  // Check the extended register, whose result is expected to be not more than
  // 0xFF leaves.
  auto reg = cpu.cpuid(0x80000000, 0);
  ASSERT_HAS_VALUE(reg);
  ASSERT_EQ(reg->eax & 0xFFFFFF00, 0x80000000);
}

TEST(cpuidreader_dev, cpuid_context_move) {
  rjcp::cpuid::cpuidreader_dev cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  unsigned int core = 0;
  if (cpu.cores() > 1) core = 1;

  auto pin = cpu.enable_core(core);
  ASSERT_HAS_VALUE(pin);
  ASSERT_TRUE(pin.value()) << ubench::string::perror(pin->error());

  auto reg0 = cpu.cpuid(1, 0);
  ASSERT_HAS_VALUE(reg0);
  auto reg0ebx = reg0.value().ebx;

  decltype(pin) pin2 = std::move(pin);
  ASSERT_HAS_VALUE(pin2);
  ASSERT_TRUE(pin2.value()) << ubench::string::perror(pin2->error());

  // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
  ASSERT_HAS_VALUE(pin);
  // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
  ASSERT_FALSE(pin.value());

  auto reg1 = cpu.cpuid(1, 0);
  ASSERT_HAS_VALUE(reg1);
  auto reg1ebx = reg1.value().ebx;

  // Moving the context should not affect which core is being read. We check
  // that the cores are the same by comparing the actual CPUID.01 EBX which is
  // unique per core.
  ASSERT_EQ(reg0ebx, reg1ebx);
}

TEST(cpuidreader_dev, cpuid_threads) {
  rjcp::cpuid::cpuidreader_dev cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  std::set<rjcp::cpuid::cpuidreg> acpi{};
  for (unsigned int core = 0; core < cpu.cores(); core++) {
    auto pin = cpu.enable_core(core);
    ASSERT_HAS_VALUE(pin);
    ASSERT_TRUE(pin.value()) << ubench::string::perror(pin->error());

    auto result = cpu.cpuid(0x00000001, 0x00000000);
    ASSERT_HAS_VALUE(result);
    EXPECT_TRUE(acpi.find(result->ebx) == acpi.end());
    acpi.insert(result->ebx);
  }
}

TEST(cpuidreader_dev, cpuid_threads_context_move) {
  rjcp::cpuid::cpuidreader_dev cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  std::set<rjcp::cpuid::cpuidreg> acpi{};
  for (unsigned int core = 0; core < cpu.cores(); core++) {
    auto pin = cpu.enable_core(core);
    ASSERT_HAS_VALUE(pin);
    ASSERT_TRUE(pin.value()) << ubench::string::perror(pin->error());

    auto pin2 = std::move(pin);

    // We've moved, but after the move. Users should not use this after the
    // move, but the check here is to make sure that the move worked so we don't
    // have latent problems with pin is destroyed

    // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    ASSERT_HAS_VALUE(pin);      // the optional has a value.
    EXPECT_FALSE(pin.value());  // the value is not valid.

    ASSERT_HAS_VALUE(pin2);
    ASSERT_TRUE(pin2.value()) << ubench::string::perror(pin->error());

    auto result = cpu.cpuid(0x00000001, 0x00000000);
    ASSERT_HAS_VALUE(result);
    EXPECT_TRUE(acpi.find(result->ebx) == acpi.end());
    acpi.insert(result->ebx);
  }
}

TEST(cpuidreader_dev, cpuid_query_then_enable) {
  rjcp::cpuid::cpuidreader_dev cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  // Query using the default core (0).
  auto regd = cpu.cpuid(1, 0);
  ASSERT_HAS_VALUE(regd);
  auto regdebx = regd->ebx;

  // Now enable a specific core, which should succeed.
  unsigned int core = 0;
  if (cpu.cores() > 1) core = 1;

  auto pin = cpu.enable_core(core);
  ASSERT_HAS_VALUE(pin);
  ASSERT_TRUE(pin.value()) << ubench::string::perror(pin->error());
  EXPECT_EQ(pin->core(), core);

  auto regc = cpu.cpuid(1, 0);
  ASSERT_HAS_VALUE(regc);
  auto regcebx = regc->ebx;

  // They should be different, unless we queried the same core.
  ASSERT_EQ(regcebx == regdebx, core == 0);
}

TEST(cpuidreader_dev, cpuid_enable_core_twice_fail) {
  rjcp::cpuid::cpuidreader_dev cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  unsigned int core = 0;
  if (cpu.cores() > 1) core = 1;

  auto pin = cpu.enable_core(core);
  ASSERT_HAS_VALUE(pin);
  ASSERT_TRUE(pin.value()) << ubench::string::perror(pin->error());
  EXPECT_EQ(pin->core(), core);

  // The second activation should fail, because the first activation is still
  // valid.

  auto pin2 = cpu.enable_core(core);
  ASSERT_FALSE(pin2.has_value());
}

TEST(cpuidreader_dev, cpuid_enable_core_twice_success) {
  rjcp::cpuid::cpuidreader_dev cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  unsigned int core = 0;
  if (cpu.cores() > 1) core = 1;

  rjcp::cpuid::cpuidreg regc = 0;
  {
    auto pin = cpu.enable_core(core);
    ASSERT_HAS_VALUE(pin);
    ASSERT_TRUE(pin.value()) << ubench::string::perror(pin->error());
    EXPECT_EQ(pin->core(), core);
    auto reg = cpu.cpuid(1, 0);
    ASSERT_HAS_VALUE(reg);
    regc = reg->ebx;
  }

  // The second activation should succeed because the first activation went out
  // of scope.
  rjcp::cpuid::cpuidreg reg0 = 0;
  {
    auto pin = cpu.enable_core(0);
    ASSERT_HAS_VALUE(pin);
    ASSERT_TRUE(pin.value()) << ubench::string::perror(pin->error());
    EXPECT_EQ(pin->core(), 0);
    auto reg = cpu.cpuid(1, 0);
    ASSERT_HAS_VALUE(reg);
    reg0 = reg->ebx;
  }

  ASSERT_EQ(regc == reg0, core == 0);
}

TEST(cpuidreader_dev, move_ctor) {
  rjcp::cpuid::cpuidreader_dev cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  unsigned int core = 0;
  if (cpu.cores() > 1) core = 1;

  rjcp::cpuid::cpuidreg regebx{}, regmebx{};
  {
    auto pin = cpu.enable_core(core);
    ASSERT_HAS_VALUE(pin);
    ASSERT_TRUE(pin.value()) << ubench::string::perror(pin->error());
    EXPECT_EQ(pin->core(), core);
    auto reg = cpu.cpuid(1, 0);
    ASSERT_HAS_VALUE(reg);
    regebx = reg->ebx;

    // Move and test we get the same results.

    decltype(cpu) cpu2{std::move(cpu)};
    auto regm = cpu2.cpuid(1, 0);
    ASSERT_HAS_VALUE(regm);
    regmebx = regm->ebx;
  }

  ASSERT_EQ(regebx, regmebx);
}

TEST(cpuidreader_dev, move_assignment) {
  rjcp::cpuid::cpuidreader_dev cpu{};
  if (!cpu.has_cpuid()) GTEST_SKIP() << "No CPUID supported";

  rjcp::cpuid::cpuidreader_dev cpu2{};

  unsigned int core = 0;
  if (cpu.cores() > 1) core = 1;

  rjcp::cpuid::cpuidreg regebx{}, regmebx{};
  {
    auto pin = cpu.enable_core(core);
    ASSERT_HAS_VALUE(pin);
    ASSERT_TRUE(pin.value()) << ubench::string::perror(pin->error());
    EXPECT_EQ(pin->core(), core);
    auto reg = cpu.cpuid(1, 0);
    ASSERT_HAS_VALUE(reg);
    regebx = reg->ebx;

    // Move and test we get the same results.

    cpu2 = std::move(cpu);
    auto regm = cpu2.cpuid(1, 0);
    ASSERT_HAS_VALUE(regm);
    regmebx = regm->ebx;
  }

  ASSERT_EQ(regebx, regmebx);

  auto pin2 = cpu2.enable_core(0);
  ASSERT_HAS_VALUE(pin2);
  ASSERT_TRUE(pin2.value()) << ubench::string::perror(pin2->error());
}
