#include "ubench/thread.h"

namespace ubench::thread {

void sync_event::set() {
  {
    std::lock_guard g(mutex_);
    flag_ = true;
  }
  cond_var_.notify_all();
}

void sync_event::clear() {
  std::lock_guard g(mutex_);
  flag_ = false;
}

void sync_event::wait() {
  std::unique_lock lock(mutex_);
  cond_var_.wait(lock, [this]() { return flag_; });
}

}  // namespace ubench::thread
