#include "sdd/subscription.h"

#include <gtest/gtest.h>

TEST(subscription, subscribe_no_copy_ctor) {
  static_assert(!std::is_copy_constructible_v<subscription>);
}

TEST(subscription, subscribe_no_copy_assignment) {
  static_assert(!std::is_copy_assignable_v<subscription>);
}

TEST(subscription, set_var_on_destructor) {
  bool cancelled = false;
  {
    subscription s([&cancelled] { cancelled = true; });
    EXPECT_FALSE(cancelled);
  }
  EXPECT_TRUE(cancelled);
}

TEST(subscription, subscribe_move_to_default_object) {
  int calls = 0;
  subscription first(nullptr);
  {
    subscription second([&calls] { ++calls; });
    first = std::move(second);
    EXPECT_EQ(calls, 0);
  }
  EXPECT_EQ(calls, 0);
}

TEST(subscription, subscribe_move_to_object_unsubscribe) {
  bool first_cancelled = false;
  int second_calls = 0;
  {
    subscription first([&first_cancelled] { first_cancelled = true; });
    {
      subscription second([&second_calls] { ++second_calls; });
      first = std::move(second);
      EXPECT_TRUE(first_cancelled);
      EXPECT_EQ(second_calls, 0);
    }
    EXPECT_EQ(second_calls, 0);
  }
  EXPECT_EQ(second_calls, 1);
}
