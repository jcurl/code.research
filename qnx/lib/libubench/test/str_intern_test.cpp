#include "ubench/str_intern.h"

#include <stdexcept>

#include <gtest/gtest.h>

TEST(str_intern, initialise) {
  auto str_intern = ubench::string::str_intern();

  EXPECT_EQ(str_intern.size(), 0);
  EXPECT_EQ(str_intern.bucket_count(), 4096);
  EXPECT_EQ(str_intern.max_load_factor(), 1.0);
}

TEST(str_intern, initialise_small) {
  auto str_intern = ubench::string::str_intern(1, 256);

  EXPECT_EQ(str_intern.size(), 0);
  EXPECT_EQ(str_intern.bucket_count(), 1);
  EXPECT_EQ(str_intern.max_load_factor(), 1.0);

  std::string teststr = "sso";
  const auto& ref = str_intern.intern(teststr);
  EXPECT_NE(&ref, &teststr);
  EXPECT_EQ(ref, teststr);
  EXPECT_EQ(str_intern.size(), 1);
  EXPECT_EQ(str_intern.bucket_count(), 1);
}

TEST(str_intern, initialise_min_max) {
  auto str_intern = ubench::string::str_intern(8192, 16384);
  EXPECT_EQ(str_intern.size(), 0);
  EXPECT_EQ(str_intern.bucket_count(), 8192);
  EXPECT_EQ(str_intern.max_load_factor(), 1.0);
}

TEST(str_intern, initialise_min_max_error) {
  EXPECT_THROW({ auto str_intern = ubench::string::str_intern(8192, 4096); },
      std::invalid_argument);
}

TEST(str_intern, initialise_zero_buckets) {
  EXPECT_THROW({ auto str_intern = ubench::string::str_intern(0, 4096); },
      std::invalid_argument);
}

TEST(str_intern, initialise_non_pow2_buckets) {
  EXPECT_THROW({ auto str_intern = ubench::string::str_intern(3, 4096); },
      std::invalid_argument);

  EXPECT_THROW({ auto str_intern = ubench::string::str_intern(2047, 4096); },
      std::invalid_argument);

  EXPECT_THROW({ auto str_intern = ubench::string::str_intern(256, 4095); },
      std::invalid_argument);
}

TEST(str_intern, initialise_not_enough_max_buckets) {
  EXPECT_THROW({ auto str_intern = ubench::string::str_intern(256, 128); },
      std::invalid_argument);
}

TEST(str_intern, add_string) {
  auto str_intern = ubench::string::str_intern();
  std::string teststr = "sso";
  const auto& ref = str_intern.intern(teststr);
  EXPECT_NE(&ref, &teststr);
  EXPECT_EQ(ref, teststr);
  EXPECT_EQ(str_intern.size(), 1);
  EXPECT_EQ(str_intern.bucket_count(), 4096);
}

TEST(str_intern, add_string_twice) {
  auto str_intern = ubench::string::str_intern();

  std::string teststr = "sso";
  const auto& ref1 = str_intern.intern(teststr);
  EXPECT_NE(&ref1, &teststr);
  EXPECT_EQ(ref1, teststr);
  EXPECT_EQ(str_intern.size(), 1);
  EXPECT_EQ(str_intern.bucket_count(), 4096);

  const auto& ref2 = str_intern.intern(teststr);
  EXPECT_NE(&ref2, &teststr);
  EXPECT_EQ(ref2, teststr);
  EXPECT_EQ(str_intern.size(), 1);
  EXPECT_EQ(str_intern.bucket_count(), 4096);

  EXPECT_EQ(&ref1, &ref2);
}

TEST(str_intern, add_string_twice_new_copy) {
  auto str_intern = ubench::string::str_intern();

  std::string teststr1 = "sso";
  const auto& ref1 = str_intern.intern(teststr1);
  EXPECT_NE(&ref1, &teststr1);
  EXPECT_EQ(ref1, teststr1);
  EXPECT_EQ(str_intern.size(), 1);
  EXPECT_EQ(str_intern.bucket_count(), 4096);

  std::string teststr2 = "sso";
  const auto& ref2 = str_intern.intern(teststr2);
  EXPECT_NE(&ref2, &teststr2);
  EXPECT_EQ(ref2, teststr2);
  EXPECT_EQ(str_intern.size(), 1);
  EXPECT_EQ(str_intern.bucket_count(), 4096);

  EXPECT_EQ(&ref1, &ref2);
}

TEST(str_intern, add_string_rehash_small) {
  auto str_intern = ubench::string::str_intern(1, 256);

  std::string teststr1 = "sso1";
  std::string teststr2 = "sso2";
  std::string teststr3 = "sso3";
  std::string teststr4 = "sso4";
  std::string teststr5 = "sso5";
  std::string teststr6 = "sso6";
  std::string teststr7 = "sso7";
  std::string teststr8 = "sso8";

  // load_factor before = 0 / 1 = 0; no rehash on insert.
  const auto& ref1 = str_intern.intern(teststr1);
  EXPECT_EQ(str_intern.size(), 1);
  EXPECT_EQ(str_intern.bucket_count(), 1);
  EXPECT_EQ(ref1, teststr1);

  // load_factor before = 1 / 1 = 1; Rehash to 2 / 1 = 2 buckets
  const auto& ref2 = str_intern.intern(teststr2);
  EXPECT_EQ(str_intern.size(), 2);
  EXPECT_EQ(str_intern.bucket_count(), 2);
  EXPECT_EQ(ref2, teststr2);

  // load_factor before = 2 / 2 = 2; Rehash to 3 / 1 = 4 buckets.
  const auto& ref3 = str_intern.intern(teststr3);
  EXPECT_EQ(str_intern.size(), 3);
  EXPECT_EQ(str_intern.bucket_count(), 4);
  EXPECT_EQ(ref3, teststr3);

  // load_factor before = 3 / 4 = 0.75; No rehash.
  const auto& ref4 = str_intern.intern(teststr4);
  EXPECT_EQ(str_intern.size(), 4);
  EXPECT_EQ(str_intern.bucket_count(), 4);
  EXPECT_EQ(ref4, teststr4);

  // load_factor before = 4 / 4 = 1; Rehash to 5 / 4 = 8 buckets.
  const auto& ref5 = str_intern.intern(teststr5);
  EXPECT_EQ(str_intern.size(), 5);
  EXPECT_EQ(str_intern.bucket_count(), 8);
  EXPECT_EQ(ref5, teststr5);

  // load_factor before = 5 / 8 = 0.68; No rehash.
  const auto& ref6 = str_intern.intern(teststr6);
  EXPECT_EQ(str_intern.size(), 6);
  EXPECT_EQ(str_intern.bucket_count(), 8);
  EXPECT_EQ(ref6, teststr6);

  // load_factor before = 6 / 8 = 0.75; No rehash.
  const auto& ref7 = str_intern.intern(teststr7);
  EXPECT_EQ(str_intern.size(), 7);
  EXPECT_EQ(str_intern.bucket_count(), 8);
  EXPECT_EQ(ref7, teststr7);

  // load_factor before = 7 / 8 = 0.83; No rehash.
  const auto& ref8 = str_intern.intern(teststr8);
  EXPECT_EQ(str_intern.size(), 8);
  EXPECT_EQ(str_intern.bucket_count(), 8);
  EXPECT_EQ(ref8, teststr8);

  const auto& x = str_intern.intern(teststr1);
  EXPECT_EQ(str_intern.size(), 8);
  EXPECT_EQ(str_intern.bucket_count(), 8);
  EXPECT_EQ(&x, &ref1);  // Check it didn't move due to a rehash.
}

TEST(str_intern, add_string_rehash_small_mlf_low) {
  auto str_intern = ubench::string::str_intern(1, 256);
  str_intern.max_load_factor(0.25);
  ASSERT_EQ(str_intern.max_load_factor(), 0.25);
  EXPECT_EQ(str_intern.size(), 0);
  EXPECT_EQ(str_intern.bucket_count(), 1);

  std::string teststr1 = "sso1";
  std::string teststr2 = "sso2";
  std::string teststr3 = "sso3";
  std::string teststr4 = "sso4";
  std::string teststr5 = "sso5";
  std::string teststr6 = "sso6";
  std::string teststr7 = "sso7";
  std::string teststr8 = "sso8";

  // load_factor before = 0 / 1 = 0; no rehash on insert.
  const auto& ref1 = str_intern.intern(teststr1);
  EXPECT_EQ(str_intern.size(), 1);
  EXPECT_EQ(str_intern.bucket_count(), 1);
  EXPECT_EQ(ref1, teststr1);

  // load_factor before = 1 / 1 = 1; Rehash to 2 / 0,25 = 8 buckets.
  const auto& ref2 = str_intern.intern(teststr2);
  EXPECT_EQ(str_intern.size(), 2);
  EXPECT_EQ(str_intern.bucket_count(), 8);
  EXPECT_EQ(ref2, teststr2);

  // load_factor before = 2 / 8 = 0.25; Rehash to 3 / 0.25 = 12 buckets (round
  // to 16)
  const auto& ref3 = str_intern.intern(teststr3);
  EXPECT_EQ(str_intern.size(), 3);
  EXPECT_EQ(str_intern.bucket_count(), 16);
  EXPECT_EQ(ref3, teststr3);

  // load_factor before = 3 / 8 = 0.38; No rehash.
  const auto& ref4 = str_intern.intern(teststr4);
  EXPECT_EQ(str_intern.size(), 4);
  EXPECT_EQ(str_intern.bucket_count(), 16);
  EXPECT_EQ(ref4, teststr4);

  // load_factor before = 4 / 16 = 0.25; Rehash to 5 / 0.25 = 20 buckets (round
  // to 32)
  const auto& ref5 = str_intern.intern(teststr5);
  EXPECT_EQ(str_intern.size(), 5);
  EXPECT_EQ(str_intern.bucket_count(), 32);
  EXPECT_EQ(ref5, teststr5);

  // load_factor before = 5 / 32 = 0.16;
  const auto& ref6 = str_intern.intern(teststr6);
  EXPECT_EQ(str_intern.size(), 6);
  EXPECT_EQ(str_intern.bucket_count(), 32);
  EXPECT_EQ(ref6, teststr6);

  // load_factor before = 6 / 32 = 0.19;
  const auto& ref7 = str_intern.intern(teststr7);
  EXPECT_EQ(str_intern.size(), 7);
  EXPECT_EQ(str_intern.bucket_count(), 32);
  EXPECT_EQ(ref7, teststr7);

  // load_factor before = 7 / 32 = 0.22;
  const auto& ref8 = str_intern.intern(teststr8);
  EXPECT_EQ(str_intern.size(), 8);
  EXPECT_EQ(str_intern.bucket_count(), 32);
  EXPECT_EQ(ref8, teststr8);

  // load_factor before = 8 / 32 = 0.25; We don't add, so no rehash.
  const auto& x = str_intern.intern(teststr1);
  EXPECT_EQ(str_intern.size(), 8);
  EXPECT_EQ(str_intern.bucket_count(), 32);
  EXPECT_EQ(&x, &ref1);  // Check it didn't move due to a rehash.
}

TEST(str_intern, add_string_rehash_small_mlf_high) {
  auto str_intern = ubench::string::str_intern(1, 256);
  str_intern.max_load_factor(4);
  ASSERT_EQ(str_intern.max_load_factor(), 4.0);

  std::string teststr1 = "sso1";
  std::string teststr2 = "sso2";
  std::string teststr3 = "sso3";
  std::string teststr4 = "sso4";
  std::string teststr5 = "sso5";
  std::string teststr6 = "sso6";
  std::string teststr7 = "sso7";
  std::string teststr8 = "sso8";

  // load_factor before = 0 / 1 = 0; no rehash on insert.
  const auto& ref1 = str_intern.intern(teststr1);
  EXPECT_EQ(str_intern.size(), 1);
  EXPECT_EQ(str_intern.bucket_count(), 1);
  EXPECT_EQ(ref1, teststr1);

  // load_factor before = 1 / 1 = 1; No rehash.
  const auto& ref2 = str_intern.intern(teststr2);
  EXPECT_EQ(str_intern.size(), 2);
  EXPECT_EQ(str_intern.bucket_count(), 1);
  EXPECT_EQ(ref2, teststr2);

  // load_factor before = 2 / 1 = 2; No rehash.
  const auto& ref3 = str_intern.intern(teststr3);
  EXPECT_EQ(str_intern.size(), 3);
  EXPECT_EQ(str_intern.bucket_count(), 1);
  EXPECT_EQ(ref3, teststr3);

  // load_factor before = 3 / 1 = 3; No rehash.
  const auto& ref4 = str_intern.intern(teststr4);
  EXPECT_EQ(str_intern.size(), 4);
  EXPECT_EQ(str_intern.bucket_count(), 1);
  EXPECT_EQ(ref4, teststr4);

  // load_factor before = 4 / 1 = 4; Rehash to 5 / 4 = 1.25 buckets (round up).
  const auto& ref5 = str_intern.intern(teststr5);
  EXPECT_EQ(str_intern.size(), 5);
  EXPECT_EQ(str_intern.bucket_count(), 2);
  EXPECT_EQ(ref5, teststr5);

  // load_factor before = 4 / 2 = 2; No rehash.
  const auto& ref6 = str_intern.intern(teststr6);
  EXPECT_EQ(str_intern.size(), 6);
  EXPECT_EQ(str_intern.bucket_count(), 2);
  EXPECT_EQ(ref6, teststr6);

  // load_factor before = 5 / 2 = 2.5; No rehash.
  const auto& ref7 = str_intern.intern(teststr7);
  EXPECT_EQ(str_intern.size(), 7);
  EXPECT_EQ(str_intern.bucket_count(), 2);
  EXPECT_EQ(ref7, teststr7);

  // load_factor before = 6 / 2 = 3; No rehash.
  const auto& ref8 = str_intern.intern(teststr8);
  EXPECT_EQ(str_intern.size(), 8);
  EXPECT_EQ(str_intern.bucket_count(), 2);
  EXPECT_EQ(ref8, teststr8);

  // load_factor before = 7 / 2 = 3.5; No rehash;
  const auto& x = str_intern.intern(teststr1);
  EXPECT_EQ(str_intern.size(), 8);
  EXPECT_EQ(str_intern.bucket_count(), 2);
  EXPECT_EQ(&x, &ref1);  // Check it didn't move due to a rehash.
}

TEST(str_intern, add_string_rehash_small_mlf_high_limit) {
  auto str_intern = ubench::string::str_intern(1, 4);
  str_intern.max_load_factor(1.25);
  ASSERT_EQ(str_intern.max_load_factor(), 1.25);

  std::string teststr1 = "sso1";
  std::string teststr2 = "sso2";
  std::string teststr3 = "sso3";
  std::string teststr4 = "sso4";
  std::string teststr5 = "sso5";
  std::string teststr6 = "sso6";
  std::string teststr7 = "sso7";
  std::string teststr8 = "sso8";

  // load_factor before = 0 / 1 = 0; no rehash on insert.
  const auto& ref1 = str_intern.intern(teststr1);
  EXPECT_EQ(str_intern.size(), 1);
  EXPECT_EQ(str_intern.bucket_count(), 1);
  EXPECT_EQ(ref1, teststr1);

  // load_factor before = 1 / 1 = 1; No rehash.
  const auto& ref2 = str_intern.intern(teststr2);
  EXPECT_EQ(str_intern.size(), 2);
  EXPECT_EQ(str_intern.bucket_count(), 1);
  EXPECT_EQ(ref2, teststr2);

  // load_factor before = 2 / 1 = 2; Rehash to 3 / 1.25 = 2.4 (round up).
  const auto& ref3 = str_intern.intern(teststr3);
  EXPECT_EQ(str_intern.size(), 3);
  EXPECT_EQ(str_intern.bucket_count(), 4);
  EXPECT_EQ(ref3, teststr3);

  // load_factor before = 3 / 4 = 0.75; No rehash.
  const auto& ref4 = str_intern.intern(teststr4);
  EXPECT_EQ(str_intern.size(), 4);
  EXPECT_EQ(str_intern.bucket_count(), 4);
  EXPECT_EQ(ref4, teststr4);

  // load_factor before = 4 / 4 = 1; No rehash.
  const auto& ref5 = str_intern.intern(teststr5);
  EXPECT_EQ(str_intern.size(), 5);
  EXPECT_EQ(str_intern.bucket_count(), 4);
  EXPECT_EQ(ref5, teststr5);

  // load_factor before = 5 / 4 = 1.2; No rehash.
  const auto& ref6 = str_intern.intern(teststr6);
  EXPECT_EQ(str_intern.size(), 6);
  EXPECT_EQ(str_intern.bucket_count(), 4);
  EXPECT_EQ(ref6, teststr6);

  // load_factor before = 6 / 4 = 1.5; Rehash, but limit reached.
  const auto& ref7 = str_intern.intern(teststr7);
  EXPECT_EQ(str_intern.size(), 7);
  EXPECT_EQ(str_intern.bucket_count(), 4);
  EXPECT_EQ(ref7, teststr7);

  // load_factor before = 7 / 4 = 1.75; Rehash, but limit reached.
  const auto& ref8 = str_intern.intern(teststr8);
  EXPECT_EQ(str_intern.size(), 8);
  EXPECT_EQ(str_intern.bucket_count(), 4);
  EXPECT_EQ(ref8, teststr8);

  const auto& x = str_intern.intern(teststr1);
  EXPECT_EQ(str_intern.size(), 8);
  EXPECT_EQ(str_intern.bucket_count(), 4);
  EXPECT_EQ(&x, &ref1);  // Check it didn't move due to a rehash.
}

TEST(str_intern, move_ctor) {
  std::string teststr1 = "sso1";
  auto str_intern1 = ubench::string::str_intern(1, 4);
  const auto& ref1 = str_intern1.intern(teststr1);
  EXPECT_EQ(str_intern1.size(), 1);
  EXPECT_EQ(str_intern1.bucket_count(), 1);
  EXPECT_EQ(ref1, teststr1);

  std::string teststr2 = "sso2";
  ubench::string::str_intern str_intern2{std::move(str_intern1)};
  EXPECT_EQ(str_intern2.size(), 1);
  EXPECT_EQ(str_intern2.bucket_count(), 1);

  const auto& x = str_intern2.intern(teststr1);
  EXPECT_EQ(str_intern2.size(), 1);
  EXPECT_EQ(str_intern2.bucket_count(), 1);
  EXPECT_EQ(&x, &ref1);

  const auto& ref2 = str_intern2.intern(teststr2);
  EXPECT_EQ(str_intern2.size(), 2);
  EXPECT_EQ(str_intern2.bucket_count(), 2);
  EXPECT_EQ(ref2, teststr2);
}

TEST(str_intern, move_assignment) {
  std::string teststr1 = "sso1";
  auto str_intern1 = ubench::string::str_intern(1, 4);
  const auto& ref1 = str_intern1.intern(teststr1);
  EXPECT_EQ(str_intern1.size(), 1);
  EXPECT_EQ(str_intern1.bucket_count(), 1);
  EXPECT_EQ(ref1, teststr1);

  std::string teststr2 = "sso2";
  ubench::string::str_intern str_intern2;
  str_intern2 = std::move(str_intern1);
  EXPECT_EQ(str_intern2.size(), 1);
  EXPECT_EQ(str_intern2.bucket_count(), 1);

  const auto& x = str_intern2.intern(teststr1);
  EXPECT_EQ(str_intern2.size(), 1);
  EXPECT_EQ(str_intern2.bucket_count(), 1);
  EXPECT_EQ(&x, &ref1);

  const auto& ref2 = str_intern2.intern(teststr2);
  EXPECT_EQ(str_intern2.size(), 2);
  EXPECT_EQ(str_intern2.bucket_count(), 2);
  EXPECT_EQ(ref2, teststr2);
}
