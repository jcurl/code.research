#include "cpuid/cpuidreader_cache.h"

#include <gtest/gtest.h>

#include "cpuid/cpuidreader_dev.h"
#include "cpuid/cpuidreader_native.h"
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

TEST(cpuidreader_cache, native_has_cpuid) {
  cpuidreader_native ncpu{};
  cpuidreader_cache cpu{ncpu};

  ASSERT_TRUE(cpu.has_cpuid());
}

TEST(cpuidreader_cache, make_cpuidreader) {
  cpuidreader_native ncpu{};
  auto cpu =
      make_cpuidreader<cpuidreader_cache>(static_cast<cpuidreader&>(ncpu));

  ASSERT_TRUE(cpu->has_cpuid());
}

TEST(cpuidreader_cache, native_cpuid_zero) {
  cpuidreader_native ncpu{};
  cpuidreader_cache cpu{ncpu};

  // Check the first register, whose result is expected to be not more than
  // 0xFF leaves.
  auto reg = cpu.cpuid(0, 0);
  ASSERT_HAS_VALUE(reg);
  ASSERT_EQ(reg->eax & 0xFFFFFF00, 0);
}

TEST(cpuidreader_cache, native_cpuid_ext) {
  cpuidreader_native ncpu{};
  cpuidreader_cache cpu{ncpu};

  // Check the extended register, whose result is expected to be not more than
  // 0xFF leaves.
  auto reg = cpu.cpuid(0x80000000, 0);
  ASSERT_HAS_VALUE(reg);
  ASSERT_EQ(reg->eax & 0xFFFFFF00, 0x80000000);
}

TEST(cpuidreader_cache, native_cpuid_threads) {
  cpuidreader_native ncpu{};
  cpuidreader_cache cpu{ncpu};

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

TEST(cpuidreader_cache, cpuid_native_compare) {
  cpuidreader_native ncpu{};
  cpuidreader_cache cpu{ncpu};

  ASSERT_EQ(cpu.cores(), ncpu.cores());

  for (unsigned int core = 0; core < cpu.cores(); core++) {
    auto ndump = cpuid_dump(ncpu, core);
    auto dump = cpuid_dump(cpu, core);

    ASSERT_HAS_VALUE(ndump);
    ASSERT_HAS_VALUE(dump);

    ASSERT_EQ(ndump->size(), dump->size());

    for (unsigned int i = 0; i < ndump->size(); i++) {
      EXPECT_EQ((*ndump)[i], (*dump)[i]);
    }
  }
}

TEST(cpuidreader_cache, native_oob_cores) {
  cpuidreader_native ncpu{};
  cpuidreader_cache cpu{ncpu};

  auto pin = cpu.enable_core(ubench::thread::thread_count());
  ASSERT_FALSE(pin.has_value());
  ASSERT_EQ(pin.error(), EINVAL);
}

TEST(cpuidreader_cache, dev_oob_cores) {
  cpuidreader_dev ncpu{};
  cpuidreader_cache cpu{ncpu};

  auto pin = cpu.enable_core(ubench::thread::thread_count());
  ASSERT_FALSE(pin.has_value());
  ASSERT_EQ(pin.error(), EINVAL);
}

class cpuidreader_fake : public cpuidreader_cache {
 public:
  cpuidreader_fake() {
    set_cores(1);
    cpuid_res res{1, 2, 3, 4};
    add_cpuid(0, 0, 0, res);
  }
};

TEST(cpuidreader_cache, fake_has_cpuid) {
  cpuidreader_fake cpu{};

  ASSERT_TRUE(cpu.has_cpuid());
}

TEST(cpuidreader_cache, fake_cores) {
  cpuidreader_fake cpu{};

  ASSERT_EQ(cpu.cores(), 1);
}

TEST(cpuidreader_cache, fake_cpuid_zero) {
  cpuidreader_fake cpu{};

  auto reg = cpu.cpuid(0, 0);
  ASSERT_HAS_VALUE(reg);
  ASSERT_EQ(reg->eax, 1);
  ASSERT_EQ(reg->ebx, 2);
  ASSERT_EQ(reg->ecx, 3);
  ASSERT_EQ(reg->edx, 4);
}

TEST(cpuidreader_cache, fake_cpuid_ext_nonexistent) {
  cpuidreader_fake cpu{};
  ASSERT_FALSE(cpu.cpuid(0x80000000, 0));
}

TEST(cpuidreader_cache, fake_oob_cores) {
  cpuidreader_fake cpu{};

  auto pin = cpu.enable_core(ubench::thread::thread_count());
  ASSERT_FALSE(pin.has_value());
  ASSERT_EQ(pin.error(), EINVAL);
}
