#ifndef UBENCH_THREAD_PIN_H
#define UBENCH_THREAD_PIN_H

#include <condition_variable>
#include <mutex>

namespace ubench::thread {

/// @brief Pin the current thread to core.
///
/// @param core The core to pin the thread to
///
/// @return true if the core was pinned, false otherwise. Use errno to get the
/// error.
auto pin_core(unsigned int core) -> bool;

/// @brief Be able to synchronise waiting for an event.
class sync_event {
 public:
  sync_event() = default;

  /// @brief Set the event, so that everyone blocked on wait() will resume.
  void set();

  /// @brief Clear the event.
  ///
  /// Clearing an event while an object is waiting on it has undefined
  /// behaviour.
  void clear();

  /// @brief Wait on another thread calling set() on this object.
  void wait();

 private:
  bool flag_{false};
  std::mutex mutex_;
  std::condition_variable cond_var_;
};

}  // namespace ubench::thread

#endif