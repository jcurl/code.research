#include "sdd/subscription.h"

#include <gtest/gtest.h>

TEST(subscription, subscribe_no_copy_ctor) {
  static_assert(!std::is_copy_constructible_v<sdd::infra::subscription>);
}

TEST(subscription, subscribe_no_copy_assignment) {
  static_assert(!std::is_copy_assignable_v<sdd::infra::subscription>);
}

TEST(subscription, set_var_on_destructor) {
  bool cancelled = false;
  {
    sdd::infra::subscription s([&cancelled]() -> void { cancelled = true; });
    EXPECT_FALSE(cancelled);
  }
  EXPECT_TRUE(cancelled);
}

TEST(subscription, subscribe_move_to_default_object) {
  int calls = 0;
  sdd::infra::subscription first(nullptr);
  {
    sdd::infra::subscription second([&calls]() -> void { ++calls; });
    first = std::move(second);
    EXPECT_EQ(calls, 0);
  }
  EXPECT_EQ(calls, 0);
}

TEST(subscription, subscribe_move_to_object_unsubscribe) {
  bool first_cancelled = false;
  int second_calls = 0;
  {
    sdd::infra::subscription first(
        [&first_cancelled]() -> void { first_cancelled = true; });
    {
      sdd::infra::subscription second(
          [&second_calls]() -> void { ++second_calls; });
      first = std::move(second);
      EXPECT_TRUE(first_cancelled);
      EXPECT_EQ(second_calls, 0);
    }
    EXPECT_EQ(second_calls, 0);
  }
  EXPECT_EQ(second_calls, 1);
}
