#ifndef UBENCH_THREAD_PIN_H
#define UBENCH_THREAD_PIN_H

#include <condition_variable>
#include <memory>
#include <mutex>

namespace ubench::thread {

/// @brief pin the current thread to the core, until destroyed.
///
/// This class can be moved, but should not be copied. Ensure that when this
/// class is destroyed, it is done so on the same thread it was pinned, else
/// undefind behaviour will occur.
class pin_core {
 public:
  /// @brief Pin the current thread to core.
  ///
  /// After assigning the current thread to the core, ensure that it was
  /// successful by checking the object boolean operator is true. If the boolean
  /// operator returns false, check the internal error with the error()
  /// function.
  ///
  /// For example, on NetBSD, it may fail with the error EPERM.
  ///
  /// @param core The core to pin the current thread to.
  pin_core(unsigned int core);
  pin_core(const pin_core&) = delete;
  auto operator=(const pin_core&) -> pin_core& = delete;
  pin_core(pin_core&&) noexcept;
  auto operator=(pin_core&&) noexcept -> pin_core&;
  ~pin_core();

  /// @brief Indicate if this thread is pinned, or if there was an error.
  operator bool() const;

  /// @brief If the thread wasn't pinned, this will return the errno value.
  ///
  /// @return The errno value from the pin operation.
  [[nodiscard]] auto error() const -> int;

  /// @brief Get the core that this thread is pinned to.
  ///
  /// @return The core that is currently pinned by this class.
  [[nodiscard]] auto core() const -> unsigned int;

 private:
  class pin_core_impl;
  std::unique_ptr<pin_core_impl> impl_;
};

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
