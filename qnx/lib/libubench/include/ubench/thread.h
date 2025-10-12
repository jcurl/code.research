#ifndef UBENCH_THREAD_PIN_H
#define UBENCH_THREAD_PIN_H

#include <condition_variable>
#include <mutex>

#include "stdext/expected.h"

namespace ubench::thread {

/// @brief Pin the current thread to core.
///
/// @param core The core to pin the thread to
///
/// @return true if the core was pinned, false otherwise. Use errno to get the
/// error.
auto pin_core(unsigned int core) -> stdext::expected<void, int>;

/// @brief Be able to synchronise waiting for an event.
class sync_event {
 public:
  sync_event() = default;

  /// @brief Set the event, so that everyone blocked on wait() will resume.
  void set() {
    {
      std::lock_guard g(mutex_);
      flag_ = true;
    }
    cond_var_.notify_all();
  }

  /// @brief Clear the event.
  ///
  /// Clearing an event while an object is waiting on it has undefined
  /// behaviour.
  void clear() {
    std::lock_guard g(mutex_);
    flag_ = false;
  }

  /// @brief Wait on another thread calling set() on this object.
  void wait() {
    std::unique_lock lock(mutex_);
    cond_var_.wait(lock, [this]() { return flag_; });
  }

  /// @brief Wait on another thread calling set() or timeout.
  ///
  /// @tparam Rep an arithmetic type, or a class emulating an arithmetic type,
  /// representing the number of ticks.
  ///
  /// @tparam Period a std::ratio representing the tick period (i.e. the number
  /// of second's fractions per tick).
  ///
  /// @return true if the condition was satisfied, false if not set().
  template <class Rep, class Period>
  [[nodiscard]] auto wait_for(
      const std::chrono::duration<Rep, Period>& rel_time) -> bool {
    std::unique_lock lock(mutex_);
    return cond_var_.wait_for(lock, rel_time, [this]() { return flag_; });
  }

 private:
  bool flag_{false};
  std::mutex mutex_;
  std::condition_variable cond_var_;
};

}  // namespace ubench::thread

#endif