#include "ubench/file.h"

#include <cstdint>
#include <type_traits>

#include <gtest/gtest.h>

auto test_file() -> const std::string& {
  static const std::string value{"/dev/null"};
  return value;
}

TEST(file, no_copy) {
  EXPECT_FALSE(std::is_copy_constructible_v<ubench::file::fdesc>);
  EXPECT_FALSE(std::is_copy_assignable_v<ubench::file::fdesc>);
}

TEST(file, has_move) {
  EXPECT_TRUE(std::is_move_constructible_v<ubench::file::fdesc>);
  EXPECT_TRUE(std::is_move_assignable_v<ubench::file::fdesc>);
}

TEST(file, default) {
  ubench::file::fdesc fd{};

  EXPECT_FALSE(fd);
  EXPECT_EQ(static_cast<int>(fd), -1);
}

TEST(file, open_file) {
  // Interop
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  ubench::file::fdesc fd = open(test_file().c_str(), O_RDONLY);

  EXPECT_TRUE(fd);
  EXPECT_NE(static_cast<int>(fd), -1);
}

TEST(file, open_file_ctor_default) {
  ubench::file::fdesc fd{test_file()};

  EXPECT_TRUE(fd);
  EXPECT_NE(static_cast<int>(fd), -1);
}

TEST(file, open_file_ctor) {
  ubench::file::fdesc fd{test_file(), O_RDWR};

  EXPECT_TRUE(fd);
  EXPECT_NE(static_cast<int>(fd), -1);
}

TEST(file, close) {
  // Interop
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  ubench::file::fdesc fd = open(test_file().c_str(), O_RDONLY);

  EXPECT_TRUE(fd);
  EXPECT_NE(static_cast<int>(fd), -1);

  fd.close();

  EXPECT_FALSE(fd);
  EXPECT_EQ(static_cast<int>(fd), -1);
}

TEST(file, move_ctor_other_invalid) {
  ubench::file::fdesc fd{test_file()};

  EXPECT_TRUE(fd);
  EXPECT_NE(static_cast<int>(fd), -1);

  ubench::file::fdesc fd2{std::move(fd)};

  // Only for testing to show the resource is clear. Shouldn't be used in code.
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_FALSE(fd);
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
  EXPECT_EQ(static_cast<int>(fd), -1);

  EXPECT_TRUE(fd2);
  EXPECT_NE(static_cast<int>(fd2), -1);
}

TEST(file, move_op_other_invalid) {
  ubench::file::fdesc fd{test_file()};

  EXPECT_TRUE(fd);
  EXPECT_NE(static_cast<int>(fd), -1);

  ubench::file::fdesc fd2 = std::move(fd);

  // Only for testing to show the resource is clear. Shouldn't be used in code.
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_FALSE(fd);
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
  EXPECT_EQ(static_cast<int>(fd), -1);

  EXPECT_TRUE(fd2);
  EXPECT_NE(static_cast<int>(fd2), -1);
}

TEST(file, reset) {
  ubench::file::fdesc fd{test_file()};

  EXPECT_TRUE(fd);
  EXPECT_NE(static_cast<int>(fd), -1);

  int native_fd = fd.reset();
  EXPECT_NE(native_fd, -1);
  EXPECT_FALSE(fd);
  EXPECT_EQ(static_cast<int>(fd), -1);

  close(native_fd);
}
