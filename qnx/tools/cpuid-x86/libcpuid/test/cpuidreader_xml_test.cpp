#include "cpuid/cpuidreader_xml.h"

#include <filesystem>

#include <gtest/gtest.h>

#include "ubench/os.h"
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

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ASSERT_HAS_NOT_VALUE(variable)       \
  {                                      \
    ASSERT_FALSE((variable).has_value()); \
    if (!(variable).has_value()) return; \
  }

auto has_cpuid_driver(const std::filesystem::path& xml_file) -> bool {
  return exists(xml_file);
}

TEST(cpuidreader_xml, has_cpuid) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "i9-12900K.xml";
  if (!has_cpuid_driver(xml_file)) GTEST_SKIP() << "No XML supported";

  cpuidreader_xml cpu{xml_file.string()};

  ASSERT_TRUE(cpu.has_cpuid());
  ASSERT_EQ(cpu.cores(), 24);
}

TEST(cpuidreader_xml, make_cpuidreader) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "i9-12900K.xml";
  if (!has_cpuid_driver(xml_file)) GTEST_SKIP() << "No XML supported";

  auto cpu = make_cpuidreader<cpuidreader_xml>(xml_file.string());

  ASSERT_TRUE(cpu->has_cpuid());
}

TEST(cpuidreader_xml, cpuid_zero) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "i9-12900K.xml";
  if (!has_cpuid_driver(xml_file)) GTEST_SKIP() << "No XML supported";

  cpuidreader_xml cpu{xml_file.string()};

  auto reg = cpu.cpuid(0, 0);
  ASSERT_HAS_VALUE(reg);
  ASSERT_EQ(reg->eax, 0x00000020);
  ASSERT_EQ(reg->ebx, 0x756E6547);
  ASSERT_EQ(reg->ecx, 0x6C65746E);
  ASSERT_EQ(reg->edx, 0x49656E69);
}

TEST(cpuidreader_xml, cpuid_ext) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "i9-12900K.xml";
  if (!has_cpuid_driver(xml_file)) GTEST_SKIP() << "No XML supported";

  cpuidreader_xml cpu{xml_file.string()};

  auto reg = cpu.cpuid(0x80000000, 0);
  ASSERT_HAS_VALUE(reg);
  ASSERT_EQ(reg->eax, 0x80000008);
  ASSERT_EQ(reg->ebx, 0x00000000);
  ASSERT_EQ(reg->ecx, 0x00000000);
  ASSERT_EQ(reg->edx, 0x00000000);
}

TEST(cpuidreader_xml, cpuid_threads) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "i9-12900K.xml";
  if (!has_cpuid_driver(xml_file)) GTEST_SKIP() << "No XML supported";

  cpuidreader_xml cpu{xml_file.string()};

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

TEST(cpuidreader_xml, oob_cores) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "i9-12900K.xml";
  if (!has_cpuid_driver(xml_file)) GTEST_SKIP() << "No XML supported";

  cpuidreader_xml cpu{xml_file.string()};

  auto pin = cpu.enable_core(24);
  ASSERT_FALSE(pin.has_value());
  ASSERT_EQ(pin.error(), EINVAL);
}

TEST(cpuidreader_xml, has_cpuid_empty_file) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "invalid_empty.xml";
  if (!has_cpuid_driver(xml_file)) GTEST_SKIP() << "No XML supported";

  cpuidreader_xml cpu{xml_file.string()};

  ASSERT_FALSE(cpu.has_cpuid());
}

TEST(cpuidreader_xml, has_cpuid_invalid_root) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "invalid_root.xml";
  if (!has_cpuid_driver(xml_file)) GTEST_SKIP() << "No XML supported";

  cpuidreader_xml cpu{xml_file.string()};

  ASSERT_FALSE(cpu.has_cpuid());
}

TEST(cpuidreader_xml, has_cpuid_no_process_nodes) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "invalid_noproc.xml";
  if (!has_cpuid_driver(xml_file)) GTEST_SKIP() << "No XML supported";

  cpuidreader_xml cpu{xml_file.string()};

  ASSERT_FALSE(cpu.has_cpuid());
}

TEST(cpuidreader_xml, has_cpuid_nonexistent_file) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "nofile.xml";

  cpuidreader_xml cpu{xml_file.string()};

  ASSERT_FALSE(cpu.has_cpuid());
}

TEST(cpuidreader_xml, one_core) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "Dell_c840.xml";
  if (!has_cpuid_driver(xml_file)) GTEST_SKIP() << "No XML supported";

  cpuidreader_xml cpu{xml_file.string()};

  ASSERT_TRUE(cpu.has_cpuid());
  ASSERT_EQ(cpu.cores(), 1);
}

auto one_core_invalid_line(std::filesystem::path xml_file) {
  if (!has_cpuid_driver(xml_file)) GTEST_SKIP() << "No XML supported";

  cpuidreader_xml cpu{xml_file.string()};

  ASSERT_TRUE(cpu.has_cpuid());
  ASSERT_EQ(cpu.cores(), 1);

  ASSERT_HAS_VALUE(cpu.cpuid(0x00000000, 0x00000000));
  ASSERT_HAS_VALUE(cpu.cpuid(0x00000001, 0x00000000));
  ASSERT_HAS_VALUE(cpu.cpuid(0x00000002, 0x00000000));
  ASSERT_HAS_VALUE(cpu.cpuid(0x80000000, 0x00000000));
  ASSERT_HAS_NOT_VALUE(cpu.cpuid(0x80000001, 0x00000000));
  ASSERT_HAS_VALUE(cpu.cpuid(0x80000002, 0x00000000));
  ASSERT_HAS_VALUE(cpu.cpuid(0x80000003, 0x00000000));
  ASSERT_HAS_VALUE(cpu.cpuid(0x80000004, 0x00000000));
}

TEST(cpuidreader_xml, one_core_invalid_line_missing_field1) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "invalid_line1.xml";
  one_core_invalid_line(std::move(xml_file));
}

TEST(cpuidreader_xml, one_core_invalid_line_ecx) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "invalid_line2.xml";
  one_core_invalid_line(std::move(xml_file));
}

TEST(cpuidreader_xml, one_core_invalid_line_noecx) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "invalid_line3.xml";
  one_core_invalid_line(std::move(xml_file));
}

TEST(cpuidreader_xml, one_core_invalid_line_eax) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "invalid_line4.xml";
  one_core_invalid_line(std::move(xml_file));
}

TEST(cpuidreader_xml, one_core_invalid_line_empty_eax) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "invalid_line5.xml";
  one_core_invalid_line(std::move(xml_file));
}

TEST(cpuidreader_xml, one_core_invalid_missing_field2) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "invalid_line6.xml";
  one_core_invalid_line(std::move(xml_file));
}

TEST(cpuidreader_xml, one_core_invalid_field_oob) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "invalid_line7.xml";
  one_core_invalid_line(std::move(xml_file));
}

auto one_core_valid_line(std::filesystem::path xml_file) {
  if (!has_cpuid_driver(xml_file)) GTEST_SKIP() << "No XML supported";

  cpuidreader_xml cpu{xml_file.string()};

  ASSERT_TRUE(cpu.has_cpuid());
  ASSERT_EQ(cpu.cores(), 1);

  ASSERT_HAS_VALUE(cpu.cpuid(0x00000000, 0x00000000));
  ASSERT_HAS_VALUE(cpu.cpuid(0x00000001, 0x00000000));
  ASSERT_HAS_VALUE(cpu.cpuid(0x00000002, 0x00000000));
  ASSERT_HAS_VALUE(cpu.cpuid(0x80000000, 0x00000000));
  ASSERT_HAS_VALUE(cpu.cpuid(0x80000001, 0x00000000));
  ASSERT_HAS_VALUE(cpu.cpuid(0x80000002, 0x00000000));
  ASSERT_HAS_VALUE(cpu.cpuid(0x80000003, 0x00000000));
  ASSERT_HAS_VALUE(cpu.cpuid(0x80000004, 0x00000000));
}

TEST(cpuidreader_xml, one_core_valid_line_space1) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "valid_line_space1.xml";
  one_core_valid_line(std::move(xml_file));
}

TEST(cpuidreader_xml, one_core_valid_line_space2) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "valid_line_space2.xml";
  one_core_valid_line(std::move(xml_file));
}

TEST(cpuidreader_xml, one_core_valid_line_space3) {
  std::filesystem::path xml_file =
      ubench::os::get_executable_path() / "test_assets" / "valid_line_space3.xml";
  one_core_valid_line(std::move(xml_file));
}
