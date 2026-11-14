#ifndef SJSON_TEST_OUTPUT_H
#define SJSON_TEST_OUTPUT_H

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#include <gtest/gtest.h>

#include "ubench/os.h"

/// @brief Path for a file written by the current test.
///
/// Files go in out/<suite>.<test>/ next to the test executable, and the folders
/// are created as needed. An existing file of the same name is deleted first.
/// The file is not deleted afterwards, so it can be inspected.
inline auto test_output_path(const std::string& file_name)
    -> std::filesystem::path {
  const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name =
      std::string{info->test_suite_name()} + "." + info->name();
  const auto dir = ubench::os::get_executable_path() / "out" / test_name;
  std::filesystem::create_directories(dir);
  const auto path = dir / file_name;
  std::error_code ec;
  std::filesystem::remove(path, ec);
  return path;
}

/// @brief A file with fixed content, written to the current test's output path.
///
/// The file is kept after the test, so it can be inspected.
class temp_file {
 public:
  temp_file(const std::string& name, std::string_view content)
      : path_{test_output_path(name)} {
    std::ofstream out{path_, std::ios::binary};
    out << content;
  }
  temp_file(const temp_file&) = delete;
  auto operator=(const temp_file&) -> temp_file& = delete;
  temp_file(temp_file&&) = delete;
  auto operator=(temp_file&&) -> temp_file& = delete;
  ~temp_file() = default;

  [[nodiscard]] auto path() const -> const std::filesystem::path& {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

#endif
