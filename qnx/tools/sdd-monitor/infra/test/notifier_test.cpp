#include "sdd/notifier.h"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>

#include <gtest/gtest.h>

class i_client {
 public:
  i_client() = default;
  i_client(const i_client&) = default;
  auto operator=(const i_client&) -> i_client& = default;
  i_client(i_client&&) = default;
  auto operator=(i_client&&) -> i_client& = default;
  virtual ~i_client() = default;

  virtual void on_notify() = 0;
};

class server {
 public:
  [[nodiscard]] auto subscribe(i_client& c) -> subscription {
    return notifier_.subscribe(c);
  }

  void notify_all(bool rethrow = false) {
    notifier_.notify([](i_client& c) { c.on_notify(); }, rethrow);
  }

 private:
  notifier<i_client> notifier_;
};

struct notify_on_client {
  void operator()(i_client& c) const { c.on_notify(); }
};

class server_explicit {
 public:
  [[nodiscard]] auto subscribe(i_client& c) -> subscription {
    return notifier_.subscribe(c);
  }

  void notify_all() { notifier_.notify(notify_on_client{}); }

 private:
  notifier<i_client> notifier_;
};

template <typename S>
class client : public i_client {
 public:
  explicit client(S& s) : subscription_(s.subscribe(*this)) {}

  void on_notify() override { ++notify_count_; }

  [[nodiscard]] auto notify_count() const -> int { return notify_count_; }

 private:
  subscription subscription_;
  int notify_count_{0};
};

TEST(notifier, get_subscription_no_notification) {
  server s;
  client c(s);
  s.notify_all();
  EXPECT_EQ(c.notify_count(), 1);
}

TEST(notifier, get_subscription_no_notification_explicit) {
  server_explicit s;
  client c(s);
  s.notify_all();
  EXPECT_EQ(c.notify_count(), 1);
}

class counting_client : public i_client {
 public:
  counting_client() = default;  // No subscription
  explicit counting_client(server& s) : subscription_(s.subscribe(*this)) {}

  void on_notify() override { ++count_; }

  [[nodiscard]] auto count() const -> int { return count_; }

 private:
  int count_{0};
  subscription subscription_;
};

TEST(notifier, notify_with_no_subscribers) {
  server s;
  s.notify_all();
  SUCCEED();
}

TEST(notifier, notify_reaches_all_subscribers) {
  server s;
  counting_client a(s);
  counting_client b(s);
  s.notify_all();
  EXPECT_EQ(a.count(), 1);
  EXPECT_EQ(b.count(), 1);
}

TEST(notifier, notify_counts_each_notification) {
  server s;
  counting_client c(s);
  s.notify_all();
  s.notify_all();
  s.notify_all();
  EXPECT_EQ(c.count(), 3);
}

TEST(notifier, subscribe_after_notify_is_not_notified) {
  server s;
  s.notify_all();
  counting_client c(s);
  EXPECT_EQ(c.count(), 0);
  s.notify_all();
  EXPECT_EQ(c.count(), 1);
}

TEST(notifier, unsubscribe_stops_notifications) {
  server s;
  counting_client c;
  subscription sub = s.subscribe(c);
  s.notify_all();
  EXPECT_EQ(c.count(), 1);

  // Moving into sub unsubscribes the old one.
  sub = subscription{};
  s.notify_all();
  EXPECT_EQ(c.count(), 1);
}

TEST(notifier, unsubscribe_one_leaves_others_subscribed) {
  server s;
  counting_client a;
  counting_client b(s);
  { subscription sub_a = s.subscribe(a); }
  s.notify_all();
  EXPECT_EQ(a.count(), 0);
  EXPECT_EQ(b.count(), 1);
}

class throwing_client : public i_client {
 public:
  throwing_client() = default;  // No subscription
  explicit throwing_client(server& s) : subscription_(s.subscribe(*this)) {}

  void on_notify() override { throw std::runtime_error("callback failed"); }

 private:
  subscription subscription_;
};

TEST(notifier, notify_exception_releases_in_flight) {
  constexpr auto timeout = std::chrono::seconds(5);

  server s;
  auto c = std::make_unique<throwing_client>(s);
  EXPECT_THROW(s.notify_all(true), std::runtime_error);

  // If in_flight_ was not released, destroying the client (which cancels its
  // subscription) would block forever.
  std::promise<void> done;
  std::future<void> finished = done.get_future();
  std::thread canceller([owned = std::move(c), &done]() mutable {
    owned.reset();
    done.set_value();
  });
  if (finished.wait_for(timeout) != std::future_status::ready) {
    // Do not detach: the blocked thread still uses `s`, which is destroyed on
    // return. Returning with a joinable thread calls std::terminate and aborts.
    ADD_FAILURE()
        << "cancel blocked: in_flight_ was not released after exception";
    return;
  }
  canceller.join();
}

TEST(notifier, callback_exception_not_rethrown_when_rethrow_false) {
  server s;
  throwing_client thrower(s);
  counting_client counter(s);

  EXPECT_NO_THROW(s.notify_all(false));
  EXPECT_EQ(counter.count(), 1);
}

TEST(notifier, callback_exception_rethrown_when_rethrow_true) {
  server s;
  throwing_client thrower(s);
  counting_client counter(s);

  EXPECT_THROW(s.notify_all(true), std::runtime_error);
  EXPECT_EQ(counter.count(), 1);
}

TEST(notifier, callback_exception_rethrown_when_rethrow_true_reversed) {
  server s;
  counting_client counter(s);
  throwing_client thrower(s);

  EXPECT_THROW(s.notify_all(true), std::runtime_error);
  EXPECT_EQ(counter.count(), 1);
}

class client_threaded : public i_client {
 public:
  explicit client_threaded(server* s) : subscription_(s->subscribe(*this)) {}

  void on_notify() override {
    std::unique_lock lock(mutex_);
    notified_ = true;
    cv_.notify_all();
    cv_.wait(lock, [this] { return resumed_; });
    ++notify_count_;
  }

  void wait_for_notification() {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] { return notified_; });
  }

  void resume() {
    std::lock_guard lock(mutex_);
    resumed_ = true;
    cv_.notify_all();
  }

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  bool notified_{false};
  bool resumed_{false};
  int notify_count_{0};
  // Declared last so it is destroyed first, before the state on_notify() uses.
  subscription subscription_;
};

TEST(notifier, unsubscribe_while_inflight) {
  constexpr auto timeout = std::chrono::milliseconds(100);

  server s;
  auto c1 = std::make_unique<client_threaded>(&s);
  auto c2 = std::make_unique<client_threaded>(&s);
  client_threaded* p1 = c1.get();
  client_threaded* p2 = c2.get();

  std::promise<void> done1;
  std::promise<void> done2;
  std::promise<void> done_notify;
  std::future<void> finished1 = done1.get_future();
  std::future<void> finished2 = done2.get_future();
  std::future<void> finished_notify = done_notify.get_future();

  std::thread thread1(
      [](std::unique_ptr<client_threaded> owned, std::promise<void>& done) {
        owned->wait_for_notification();
        owned.reset();
        done.set_value();
      },
      std::move(c1), std::ref(done1));

  std::thread thread2(
      [](std::unique_ptr<client_threaded> owned, std::promise<void>& done) {
        owned->wait_for_notification();
        owned.reset();
        // netbsd10.1 clang only (clang-analyzer-core.CallAndMessage).
        //
        // The analyzer reports "Called function pointer is null" inside the
        // NetBSD sysroot's libstdc++ gthr-default.h, reached through
        // std::promise::set_value -> std::call_once -> __gthread_once. The
        // weak pthread_once symbol is null only when the program is not
        // linked with pthreads. This test always runs with threads active,
        // so the call is safe. The analyzer does not model the weak-symbol
        // check, which makes this a false positive.
        //
        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
        done.set_value();
      },
      std::move(c2), std::ref(done2));

  std::thread notifier_thread([&s, &done_notify] {
    s.notify_all();
    done_notify.set_value();
  });

  p1->wait_for_notification();
  p1->resume();
  p2->wait_for_notification();

  std::this_thread::sleep_for(timeout);

  // At this point in time, c1 is in the destructor, waiting for inflight
  // notifications to complete.
  EXPECT_NE(
      finished1.wait_for(std::chrono::seconds(0)), std::future_status::ready);
  EXPECT_NE(
      finished2.wait_for(std::chrono::seconds(0)), std::future_status::ready);
  EXPECT_NE(finished_notify.wait_for(std::chrono::seconds(0)),
      std::future_status::ready);

  p2->resume();

  // Now c2 will complete, and the destructors for all subscribers complete
  ASSERT_EQ(finished1.wait_for(timeout), std::future_status::ready)
      << "thread1 did not finish";
  ASSERT_EQ(finished2.wait_for(timeout), std::future_status::ready)
      << "thread2 did not finish";
  ASSERT_EQ(finished_notify.wait_for(timeout), std::future_status::ready)
      << "notify thread did not finish";

  // The ASSERT_EQ will cause the function to exit without getting here, so the
  // threads will raise std::terminate. Thus, we're guaranteed at this time the
  // threads did complete.
  thread1.join();
  thread2.join();
  notifier_thread.join();
}
