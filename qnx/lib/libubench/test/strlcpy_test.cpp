#include <array>

#include <gtest/gtest.h>

#include "stringext.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays,cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-constant-array-index)

static constexpr const char src[] = "source";

TEST(strlcpy, dst_zero) {
  std::vector<char> dst(10, 'x');
  auto l = strlcpy(dst.data(), src, 0);
  ASSERT_EQ(l, std::strlen(src));

  for (size_t i = 0; i < dst.size(); ++i) {
    ASSERT_EQ(dst[i], 'x') << "i = " << i << " dst modified: " << dst[i];
  }
}

TEST(strlcpy, dst_less_src) {
  std::vector<char> dst(10, 'x');

  auto l = strlcpy(dst.data(), src, strlen(src) - 1);
  ASSERT_EQ(l, std::strlen(src));

  for (size_t i = 0; i < strlen(src) - 2; ++i) {
    ASSERT_EQ(dst[i], src[i]) << "i = " << i << " dst != src";
  }
  for (size_t i = strlen(src) - 1; i < dst.size(); ++i) {
    ASSERT_EQ(dst[i], 'x') << "i = " << i << " dst modified: " << dst[i];
  }
}

TEST(strlcpy, dst_len_src_no_nul) {
  std::vector<char> dst(10, 'x');

  auto l = strlcpy(dst.data(), src, strlen(src));
  ASSERT_EQ(l, std::strlen(src));

  for (size_t i = 0; i < strlen(src) - 1; ++i) {
    ASSERT_EQ(dst[i], src[i]) << "i = " << i << " dst != src";
  }
  for (size_t i = strlen(src); i < dst.size(); ++i) {
    ASSERT_EQ(dst[i], 'x') << "i = " << i << " dst modified: " << dst[i];
  }
}

TEST(strlcpy, dst_len_src_with_nul) {
  std::vector<char> dst(10, 'x');

  auto l = strlcpy(dst.data(), src, strlen(src) + 1);
  ASSERT_EQ(l, std::strlen(src));

  for (size_t i = 0; i < strlen(src); ++i) {
    ASSERT_EQ(dst[i], src[i]) << "i = " << i << " dst != src";
  }
  for (size_t i = strlen(src) + 1; i < dst.size(); ++i) {
    ASSERT_EQ(dst[i], 'x') << "i = " << i << " dst modified: " << dst[i];
  }
}

TEST(strlcpy, dst_len_full) {
  std::vector<char> dst(10, 'x');

  auto l = strlcpy(dst.data(), src, dst.size());
  ASSERT_EQ(l, std::strlen(src));

  for (size_t i = 0; i < strlen(src); ++i) {
    ASSERT_EQ(dst[i], src[i]) << "i = " << i << " dst != src";
  }

  // Unlike strncpy, remaining byte aren't zeroed.
  for (size_t i = strlen(src) + 1; i < dst.size(); ++i) {
    ASSERT_EQ(dst[i], 'x') << "i = " << i << " dst modified: " << dst[i];
  }
}

// NOLINTEND(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays,cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-constant-array-index)
