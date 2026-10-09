#include "config.h"

#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdint>
#include <iomanip>

#include <gtest/gtest.h>

#include "cpuid/cpuid.h"
#include "ubench/file.h"
#include "ubench/string.h"
#include "ubench/thread.h"
#include "cpuid_dev.h"

#if HAVE_CPUIDX86
#include <cpuid.h>
#endif

namespace {

static constexpr rjcp::cpuid::cpuid_res invalid_cpuid_res = {
    .eax = 0xFFFFFFFF, .ebx = 0xFFFFFFFF, .ecx = 0xFFFFFFFF, .edx = 0xFFFFFFFF};

// Prefer lseek64/pread64 when declared (32-bit off_t); SFINAE drops them
// otherwise. The int/long argument ranks the preferred overload first.
template <typename T>
auto cpuid_lseek_impl(int fd, T offset, int whence, int)
    -> decltype(lseek64(fd, offset, whence)) {
  return lseek64(fd, offset, whence);
}

template <typename T>
auto cpuid_lseek_impl(int fd, T offset, int whence, long) -> off_t {
  static_assert(sizeof(off_t) >= sizeof(std::uint64_t),
      "64-bit file offsets are required");
  return ::lseek(fd, static_cast<off_t>(offset), whence);
}

auto cpuid_lseek(int fd, std::uint64_t offset, int whence) {
  return cpuid_lseek_impl(fd, offset, whence, 0);
}

template <typename T>
auto cpuid_pread_impl(int fd, void* buf, std::size_t len, T offset, int)
    -> decltype(pread64(fd, buf, len, offset)) {
  return pread64(fd, buf, len, offset);
}

template <typename T>
auto cpuid_pread_impl(int fd, void* buf, std::size_t len, T offset, long)
    -> ssize_t {
  static_assert(sizeof(off_t) >= sizeof(std::uint64_t),
      "64-bit file offsets are required");
  return ::pread(fd, buf, len, static_cast<off_t>(offset));
}

auto cpuid_pread(int fd, void* buf, std::size_t len, std::uint64_t offset) {
  return cpuid_pread_impl(fd, buf, len, offset, 0);
}

auto get_cpuid_read(ubench::file::fdesc& fd, rjcp::cpuid::cpuid_req req)
    -> rjcp::cpuid::cpuid_res {
  if (!fd) return invalid_cpuid_res;

  auto lseekr = cpuid_lseek(
      fd, req.eax | static_cast<std::uint64_t>(req.ecx) << 32, SEEK_SET);
  if (lseekr == -1) {
    ubench::string::perror(std::string{"lseek"});
    return invalid_cpuid_res;
  }

  std::array<std::uint32_t, 4> reg{};
  auto readr = read(fd, reg.data(), 16);
  if (readr == -1) {
    ubench::string::perror(std::string{"read"});
    return invalid_cpuid_res;
  }
  if (readr != 16) {
    std::cout << "read invalid return: " << readr << std::endl;
    return invalid_cpuid_res;
  }
  return {.eax = reg[0], .ebx = reg[1], .ecx = reg[2], .edx = reg[3]};
}

auto get_cpuid_pread(ubench::file::fdesc& fd, rjcp::cpuid::cpuid_req req)
    -> rjcp::cpuid::cpuid_res {
  if (!fd) return invalid_cpuid_res;

  std::array<std::uint32_t, 4> reg{};
  auto preadr = cpuid_pread(
      fd, reg.data(), 16, req.eax | static_cast<std::uint64_t>(req.ecx) << 32);
  if (preadr == -1) {
    ubench::string::perror(std::string{"preadr"});
    return invalid_cpuid_res;
  }
  if (preadr != 16) {
    std::cout << "read invalid return: " << preadr << std::endl;
    return invalid_cpuid_res;
  }
  return {.eax = reg[0], .ebx = reg[1], .ecx = reg[2], .edx = reg[3]};
}

auto get_cpuid_native([[maybe_unused]] unsigned int core,
    [[maybe_unused]] rjcp::cpuid::cpuid_req req) -> rjcp::cpuid::cpuid_res {
#if HAVE_CPUIDX86
  struct rjcp::cpuid::cpuid_res res {};

  auto pin = ubench::thread::pin_core(core);
  __cpuid_count(req.eax, req.ecx, res.eax, res.ebx, res.ecx, res.edx);
  return res;
#else
  return invalid_cpuid_res;
#endif
}

}  // namespace

TEST(cpuid_dev_mod, has_cpuid) {
  auto fd = get_cpuid_dev_fdpath(0);
  if (!fd) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }
}

TEST(cpuid_dev_mod, cpu_core_count) {
  if (!has_cpuid_dev_driver()) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  unsigned int cores = ubench::thread::thread_count();
  for (unsigned int i = 0; i < cores; i++) {
    auto fd = get_cpuid_dev_fdpath(i);
    ASSERT_TRUE(fd) << "Core " << i << " not found or not readable";
  }
}

TEST(cpuid_dev_mod, compare_mod_read) {
  if (!has_cpuid_dev_driver()) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  unsigned int cores = ubench::thread::thread_count();
  for (unsigned int i = 0; i < cores; i++) {
    auto basecpuid = get_cpuid_native(i, {0x00000000, 0x00000000});
    EXPECT_NE(basecpuid.eax, 0xFFFFFFFF);
    if (basecpuid.eax != 0xFFFFFFFF) {
      auto fd = get_cpuid_dev_fdpath(i);
      for (unsigned int n = 0; n <= basecpuid.eax; n++) {
        auto native = get_cpuid_native(i, {n, 0});
        auto cpuid_read = get_cpuid_read(fd, {n, 0});
        EXPECT_EQ(native.eax, cpuid_read.eax)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.ebx, cpuid_read.ebx)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.ecx, cpuid_read.ecx)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.edx, cpuid_read.edx)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
      }
    }
    auto extcpuid = get_cpuid_native(i, {0x80000000, 0x00000000});
    EXPECT_NE(extcpuid.eax, 0xFFFFFFFF);
    if (extcpuid.eax != 0xFFFFFFFF) {
      auto fd = get_cpuid_dev_fdpath(i);
      for (unsigned int n = 0x80000000; n <= extcpuid.eax; n++) {
        auto native = get_cpuid_native(i, {n, 0});
        auto cpuid_read = get_cpuid_read(fd, {n, 0});
        EXPECT_EQ(native.eax, cpuid_read.eax)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.ebx, cpuid_read.ebx)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.ecx, cpuid_read.ecx)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.edx, cpuid_read.edx)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
      }
    }
  }
}

TEST(cpuid_dev_mod, compare_mod_pread) {
  if (!has_cpuid_dev_driver()) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  unsigned int cores = ubench::thread::thread_count();
  for (unsigned int i = 0; i < cores; i++) {
    auto basecpuid = get_cpuid_native(i, {0x00000000, 0x00000000});
    EXPECT_NE(basecpuid.eax, 0xFFFFFFFF);
    if (basecpuid.eax != 0xFFFFFFFF) {
      auto fd = get_cpuid_dev_fdpath(i);
      for (unsigned int n = 0; n <= basecpuid.eax; n++) {
        auto native = get_cpuid_native(i, {n, 0});
        auto cpuid_read = get_cpuid_pread(fd, {n, 0});
        EXPECT_EQ(native.eax, cpuid_read.eax)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.ebx, cpuid_read.ebx)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.ecx, cpuid_read.ecx)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.edx, cpuid_read.edx)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
      }
    }
    auto extcpuid = get_cpuid_native(i, {0x80000000, 0x00000000});
    EXPECT_NE(extcpuid.eax, 0xFFFFFFFF);
    if (extcpuid.eax != 0xFFFFFFFF) {
      auto fd = get_cpuid_dev_fdpath(i);
      for (unsigned int n = 0x80000000; n <= extcpuid.eax; n++) {
        auto native = get_cpuid_native(i, {n, 0});
        auto cpuid_read = get_cpuid_pread(fd, {n, 0});
        EXPECT_EQ(native.eax, cpuid_read.eax)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.ebx, cpuid_read.ebx)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.ecx, cpuid_read.ecx)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.edx, cpuid_read.edx)
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
      }
    }
  }
}

TEST(cpuid_dev_mod, compare_mod_read_multiple) {
  if (!has_cpuid_dev_driver()) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  unsigned int cores = ubench::thread::thread_count();
  for (unsigned int i = 0; i < cores; i++) {
    auto basecpuid = get_cpuid_native(i, {0x00000000, 0x00000000});
    EXPECT_NE(basecpuid.eax, 0xFFFFFFFF);
    if (basecpuid.eax != 0xFFFFFFFF) {
      auto fd = get_cpuid_dev_fdpath(i);

      // To avoid `performing an implicit widening conversion to type
      // 'size_type'`
      using regs_size_type = std::vector<std::uint32_t>::size_type;
      std::vector<std::uint32_t> regs{};
      regs.resize(4 * (static_cast<regs_size_type>(basecpuid.eax) + 1));
      auto lseekr = cpuid_lseek(fd, SEEK_SET, 0);
      ASSERT_EQ(lseekr, 0);
      auto readr = read(fd, regs.data(), regs.size() * sizeof(std::uint32_t));
      ASSERT_EQ(readr, regs.size() * sizeof(std::uint32_t));

      // Note that reading from offset zero, only reads the EAX registers, as if
      // ECX is zero.
      for (unsigned int n = 0; n <= basecpuid.eax; n++) {
        auto native = get_cpuid_native(i, {n, 0});
        auto idx = static_cast<regs_size_type>(n) * 4;
        EXPECT_EQ(native.eax, regs[idx])
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.ebx, regs[idx + 1])
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.ecx, regs[idx + 2])
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.edx, regs[idx + 3])
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
      }
    }
  }
}

TEST(cpuid_dev_mod, compare_mod_pread_multiple) {
  if (!has_cpuid_dev_driver()) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  unsigned int cores = ubench::thread::thread_count();
  for (unsigned int i = 0; i < cores; i++) {
    auto basecpuid = get_cpuid_native(i, {0x00000000, 0x00000000});
    EXPECT_NE(basecpuid.eax, 0xFFFFFFFF);
    if (basecpuid.eax != 0xFFFFFFFF) {
      auto fd = get_cpuid_dev_fdpath(i);

      // To avoid `performing an implicit widening conversion to type
      // 'size_type'`
      using regs_size_type = std::vector<std::uint32_t>::size_type;
      std::vector<std::uint32_t> regs{};
      regs.resize(4 * (static_cast<regs_size_type>(basecpuid.eax) + 1));
      auto preadr =
          cpuid_pread(fd, regs.data(), regs.size() * sizeof(std::uint32_t), 0);
      ASSERT_EQ(preadr, regs.size() * sizeof(std::uint32_t));

      // Note that reading from offset zero, only reads the EAX registers, as if
      // ECX is zero.
      for (unsigned int n = 0; n <= basecpuid.eax; n++) {
        auto native = get_cpuid_native(i, {n, 0});
        auto idx = static_cast<regs_size_type>(n) * 4;
        EXPECT_EQ(native.eax, regs[idx])
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.ebx, regs[idx + 1])
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.ecx, regs[idx + 2])
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.edx, regs[idx + 3])
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
      }
    }
  }
}

TEST(cpuid_dev_mod, compare_mod_read_ecx) {
  if (!has_cpuid_dev_driver()) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  unsigned int cores = ubench::thread::thread_count();
  for (unsigned int i = 0; i < cores; i++) {
    auto basecpuid = get_cpuid_native(i, {0x00000000, 0x00000000});
    EXPECT_NE(basecpuid.eax, 0xFFFFFFFF);
    if (basecpuid.eax != 0xFFFFFFFF && basecpuid.eax >= 4) {
      auto fd = get_cpuid_dev_fdpath(i);

      for (unsigned int n = 0; n < 4; n++) {
        auto native = get_cpuid_native(i, {4, n});
        auto cpuid_read = get_cpuid_read(fd, {4, n});
        EXPECT_EQ(native.eax, cpuid_read.eax)
            << "Core " << i << ", CPUID 0x00000004, 0x" << std::hex
            << std::setfill('0') << std::setw(8) << n;
        EXPECT_EQ(native.ebx, cpuid_read.ebx)
            << "Core " << i << ", 0x00000004, CPUID 0x" << std::hex
            << std::setfill('0') << std::setw(8) << n;
        EXPECT_EQ(native.ecx, cpuid_read.ecx)
            << "Core " << i << ", 0x00000004, CPUID 0x" << std::hex
            << std::setfill('0') << std::setw(8) << n;
        EXPECT_EQ(native.edx, cpuid_read.edx)
            << "Core " << i << ", 0x00000004, CPUID 0x" << std::hex
            << std::setfill('0') << std::setw(8) << n;
      }
    }
  }
}

TEST(cpuid_dev_mod, compare_mod_pread_ecx) {
  if (!has_cpuid_dev_driver()) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  unsigned int cores = ubench::thread::thread_count();
  for (unsigned int i = 0; i < cores; i++) {
    auto basecpuid = get_cpuid_native(i, {0x00000000, 0x00000000});
    EXPECT_NE(basecpuid.eax, 0xFFFFFFFF);
    if (basecpuid.eax != 0xFFFFFFFF && basecpuid.eax >= 4) {
      auto fd = get_cpuid_dev_fdpath(i);

      for (unsigned int n = 0; n < 4; n++) {
        auto native = get_cpuid_native(i, {4, n});
        auto cpuid_read = get_cpuid_pread(fd, {4, n});
        EXPECT_EQ(native.eax, cpuid_read.eax)
            << "Core " << i << ", CPUID 0x00000004, 0x" << std::hex
            << std::setfill('0') << std::setw(8) << n;
        EXPECT_EQ(native.ebx, cpuid_read.ebx)
            << "Core " << i << ", 0x00000004, CPUID 0x" << std::hex
            << std::setfill('0') << std::setw(8) << n;
        EXPECT_EQ(native.ecx, cpuid_read.ecx)
            << "Core " << i << ", 0x00000004, CPUID 0x" << std::hex
            << std::setfill('0') << std::setw(8) << n;
        EXPECT_EQ(native.edx, cpuid_read.edx)
            << "Core " << i << ", 0x00000004, CPUID 0x" << std::hex
            << std::setfill('0') << std::setw(8) << n;
      }
    }
  }
}

TEST(cpuid_dev_mod, compare_mod_read_offset) {
  if (!has_cpuid_dev_driver()) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  unsigned int cores = ubench::thread::thread_count();
  for (unsigned int i = 0; i < cores; i++) {
    auto basecpuid = get_cpuid_native(i, {0x00000000, 0x00000000});
    EXPECT_NE(basecpuid.eax, 0xFFFFFFFF);
    if (basecpuid.eax != 0xFFFFFFFF) {
      auto fd = get_cpuid_dev_fdpath(i);
      auto lseekr = cpuid_lseek(fd, 0, SEEK_SET);
      ASSERT_NE(lseekr, -1);

      // The Linux Kernel module increments the position by 1
      // when reading the CPUID information. We test this, that other
      // implementations do the same.
      std::vector<std::uint32_t> regs{};
      regs.resize(4);

      auto seek1 = cpuid_lseek(fd, 0, SEEK_CUR);
      auto readr = read(fd, regs.data(), regs.size() * sizeof(std::uint32_t));
      ASSERT_EQ(readr, regs.size() * sizeof(std::uint32_t));
      auto seek2 = cpuid_lseek(fd, 0, SEEK_CUR);
      EXPECT_EQ(seek1, 0);
      EXPECT_EQ(seek2, 1);

      // Now to prove that the reads are correct. We only seek once, at the
      // beginning of the loop.
      lseekr = cpuid_lseek(fd, 0, SEEK_SET);
      ASSERT_NE(lseekr, -1);
      for (unsigned int n = 0; n <= basecpuid.eax; n++) {
        auto native = get_cpuid_native(i, {n, 0});
        readr = read(fd, regs.data(), regs.size() * sizeof(std::uint32_t));
        ASSERT_EQ(readr, regs.size() * sizeof(std::uint32_t));
        EXPECT_EQ(native.eax, regs[0])
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.ebx, regs[1])
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.ecx, regs[2])
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
        EXPECT_EQ(native.edx, regs[3])
            << "Core " << i << ", CPUID 0x" << std::hex << std::setfill('0')
            << std::setw(8) << n;
      }
    }
  }
}

TEST(cpuid_dev_mod, read_zero) {
  if (!has_cpuid_dev_driver()) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  unsigned int cores = ubench::thread::thread_count();
  for (unsigned int i = 0; i < cores; i++) {
    auto fd = get_cpuid_dev_fdpath(i);
    auto lseekr = cpuid_lseek(fd, 0, SEEK_SET);
    ASSERT_NE(lseekr, -1) << "core " << i;

    std::vector<std::uint32_t> regs{};
    regs.resize(4);
    auto readr = read(fd, regs.data(), 0);
    EXPECT_EQ(readr, 0) << "core " << i;
  }
}

TEST(cpuid_dev_mod, read_under_16_bytes) {
  if (!has_cpuid_dev_driver()) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  unsigned int cores = ubench::thread::thread_count();
  for (unsigned int i = 0; i < cores; i++) {
    auto fd = get_cpuid_dev_fdpath(i);
    for (std::size_t l = 1; l < 16; l++) {
      auto lseekr = cpuid_lseek(fd, 0, SEEK_SET);
      ASSERT_NE(lseekr, -1) << "core " << i;

      std::vector<std::uint32_t> regs{};
      regs.resize(4);
      auto readr = read(fd, regs.data(), l);
      int e = errno;
      EXPECT_EQ(readr, -1) << "core " << i << ", length " << l;
      EXPECT_EQ(e, EINVAL) << "core " << i;
    }
  }
}

TEST(cpuid_dev_mod, read_under_32_bytes) {
  if (!has_cpuid_dev_driver()) {
    GTEST_SKIP() << "Couldn't open '/dev/cpu/0/cpuid'";
  }

  unsigned int cores = ubench::thread::thread_count();
  for (unsigned int i = 0; i < cores; i++) {
    auto fd = get_cpuid_dev_fdpath(i);
    for (std::size_t l = 17; l < 32; l++) {
      auto lseekr = cpuid_lseek(fd, 0, SEEK_SET);
      ASSERT_NE(lseekr, -1) << "core " << i;

      std::vector<std::uint32_t> regs{};
      regs.resize(8);
      auto readr = read(fd, regs.data(), l);
      int e = errno;
      EXPECT_EQ(readr, -1) << "core " << i << ", length " << l;
      EXPECT_EQ(e, EINVAL) << "core " << i;
    }
  }
}
