#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "sdd/computer.h"
#include "sdd/i_main_view.h"
#include "sdd/i_view_listener.h"

namespace sdd {

class main_presenter final : public i_view_listener {
 public:
  explicit main_presenter(i_main_view& view);
  ~main_presenter();

  main_presenter(const main_presenter&) = delete;
  main_presenter& operator=(const main_presenter&) = delete;

  // Starts the mock worker thread. Stand-in for the real library.
  void start();

  void on_selected(std::optional<computer_id> id) override;

 private:
  void run();

  i_main_view& view_;
  std::vector<computer> computers_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<computer_id> requests_;
  bool stop_requested_{false};  // Guarded by mutex_.

  // Joined in the destructor before the members it uses are destroyed.
  std::thread worker_;
};

}  // namespace sdd
