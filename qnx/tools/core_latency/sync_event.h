#ifndef SYNC_EVENT_H
#define SYNC_EVENT_H

#include <condition_variable>
#include <mutex>

class sync_event {
 public:
  sync_event() = default;

  void set();

  void clear();

  void wait();

 private:
  bool flag_{false};
  std::mutex mutex_;
  std::condition_variable cond_var_;
};

#endif