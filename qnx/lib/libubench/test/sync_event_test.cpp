#include <pthread.h>
#include <semaphore.h>

#include <chrono>
#include <thread>
#include <type_traits>

#include <gtest/gtest.h>

#include "ubench/thread.h"

// @brief Use Posix semaphores for synchronisation
class sem final {
 public:
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  sem() { sem_init(&sem_, 0, 0); }
  ~sem() { sem_destroy(&sem_); }

  sem(sem&) = delete;
  auto operator=(const sem&) -> sem& = delete;

  sem(sem&&) = delete;
  auto operator=(sem&&) -> sem& = delete;

  auto post() -> void { sem_post(&sem_); }
  auto wait() -> void { sem_wait(&sem_); }

 private:
  sem_t sem_;
};

TEST(sync_event, no_copy_constructor) {
  // Can't copy, because a mutex isn't copyable.
  ASSERT_FALSE(std::is_copy_constructible_v<ubench::thread::sync_event>);
}

TEST(sync_event, no_copy_assignment) {
  // Can't copy, because a mutex isn't copyable.
  ASSERT_FALSE(std::is_copy_assignable_v<ubench::thread::sync_event>);
}

TEST(sync_event, move_constructor) {
  // Can't move, because a mutex isn't movable.
  ASSERT_FALSE(std::is_move_constructible_v<ubench::thread::sync_event>);
}

TEST(sync_event, move_assignment) {
  // Can't move, because a mutex isn't movable.
  ASSERT_FALSE(std::is_move_assignable_v<ubench::thread::sync_event>);
}

TEST(sync_event, normal) {
  sem t{};
  ubench::thread::sync_event sync{};

  std::thread trigger([&sync, &t]() {
    t.wait();

    // The minor sleep helps that the wait is entered.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sync.set();
  });

  // Let the thread know we're waiting.
  t.post();
  sync.wait();
  trigger.join();
}

TEST(sync_event, preset) {
  ubench::thread::sync_event sync{};

  sync.set();
  sync.wait();
}

TEST(sync_event, preset_wait_twice) {
  ubench::thread::sync_event sync{};

  sync.set();
  sync.wait();
  sync.wait();
}

TEST(sync_event, preset_wait_clear) {
  sem t{};
  ubench::thread::sync_event sync{};

  sync.set();
  sync.wait();
  sync.clear();

  std::thread trigger([&sync, &t]() {
    t.wait();

    // The minor sleep helps that the wait is entered.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sync.set();
  });

  // Let the thread know we're waiting.
  t.post();
  sync.wait();
  trigger.join();
}

TEST(sync_event, set_clear_wait) {
  sem t{};
  ubench::thread::sync_event sync{};

  sync.set();
  sync.clear();

  std::thread trigger([&sync, &t]() {
    t.wait();

    // The minor sleep helps that the wait is entered.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sync.set();
  });

  // Let the thread know we're waiting.
  t.post();
  sync.wait();
  trigger.join();
}

TEST(sync_event, timeout_wait) {
  ubench::thread::sync_event sync{};

  ASSERT_FALSE(sync.wait_for(std::chrono::milliseconds{10}));
}

TEST(sync_event, normal_no_timeout) {
  sem t{};
  ubench::thread::sync_event sync{};

  std::thread trigger([&sync, &t]() {
    t.wait();

    // The minor sleep helps that the wait is entered.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sync.set();
  });

  // Let the thread know we're waiting.
  t.post();
  auto result = sync.wait_for(std::chrono::seconds(1));
  trigger.join();

  ASSERT_TRUE(result);
}
