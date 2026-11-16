#include "sdd/main_presenter.h"

#include <algorithm>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace sdd {
namespace {

constexpr auto announce_interval = 500ms;
constexpr auto query_latency = 400ms;

std::vector<computer> make_fake_computers() {
  return {
      {.id = 1,
          .name = "workstation-01",
          .os = "Ubuntu 26.04 LTS",
          .interfaces = {{.name = "eth0",
                             .mac = "00:1a:2b:3c:4d:5e",
                             .ips = {"192.168.1.10", "fd00::10"}},
              {.name = "wlan0",
                  .mac = "00:1a:2b:3c:4d:5f",
                  .ips = {"192.168.1.11"}}}},
      {.id = 2,
          .name = "build-server",
          .os = "QNX 8.0",
          .interfaces = {{.name = "eth0",
              .mac = "00:1a:2b:3c:4d:60",
              .ips = {"10.0.0.5"}}}},
  };
}

}  // namespace

main_presenter::main_presenter(i_main_view& view)
    : view_{view}, computers_{make_fake_computers()} {}

main_presenter::~main_presenter() {
  {
    std::scoped_lock lock{mutex_};
    stop_requested_ = true;
  }
  cv_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }
}

void main_presenter::start() {
  worker_ = std::thread{[this] { run(); }};
}

void main_presenter::on_selected(std::optional<computer_id> id) {
  if (!id) {
    return;
  }
  {
    std::scoped_lock lock{mutex_};
    requests_.push_back(*id);
  }
  cv_.notify_one();
}

void main_presenter::run() {
  // Stand-in for discovery: announce each computer by id and name only.
  for (const computer& c : computers_) {
    std::this_thread::sleep_for(announce_interval);
    {
      std::scoped_lock lock{mutex_};
        return;
      }
    }
    view_.show_computer(computer{.id = c.id, .name = c.name});
  }

  while (true) {
    computer_id id{};
    {
      std::unique_lock lock{mutex_};
      cv_.wait(lock, [this] { return stop_requested_ || !requests_.empty(); });
      if (stop_requested_) {
        return;
      }
      id = requests_.front();
      requests_.pop_front();
    }
    std::this_thread::sleep_for(query_latency);  // Stand-in for a remote query.
    const auto it = std::find_if(computers_.begin(), computers_.end(),
                                 [id](const computer& c) { return c.id == id; });
    if (it != computers_.end()) {
      view_.show_computer(*it);
    }
  }
}

}  // namespace sdd
