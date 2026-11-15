#include "cli_presenter.h"

#include <csignal>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <ctime>
#include <iostream>

namespace sdd::cli {

namespace {

constexpr auto check_period = std::chrono::seconds{5};
constexpr std::time_t lost_after = 30;

}  // namespace

cli_presenter::cli_presenter(core::computer_service& service)
    : subscription_{service.subscribe(*this)} {}

auto cli_presenter::run() -> int {
  sigset_t signals;
  sigfillset(&signals);
  timespec timeout{static_cast<time_t>(check_period.count()), 0};
  while (sigtimedwait(&signals, nullptr, &timeout) == -1 && errno == EAGAIN) {
    std::scoped_lock lock{mutex_};
    expire_lost_computers();
  }
  return termination_error_ ? 1 : 0;
}

/// @brief Reports and forgets each computer not seen within the lost limit.
/// Must be called with mutex_ held.
auto cli_presenter::expire_lost_computers() -> void {
  const auto now = std::time(nullptr);
  for (auto it = seen_.begin(); it != seen_.end();) {
    if (now - it->second.last_seen > lost_after) {
      std::cout << "lost " << it->second.name << '\n' << std::flush;
      it = seen_.erase(it);
    } else {
      ++it;
    }
  }
}

auto cli_presenter::on_computer(const core::computer& computer) -> void {
  // An exception (out of memory, or a failed write to std::cout) does not
  // corrupt state, but the notification may be lost: a computer whose details
  // were not fully printed stays recorded as seen, so it is not printed again
  // until it is lost and returns.
  std::scoped_lock lock{mutex_};
  const auto [it, is_new] = seen_.try_emplace(
      computer.id,
      seen_computer{computer.hostname + '.' + computer.domain,
          computer.last_seen});
  if (!is_new) {
    it->second.last_seen = computer.last_seen;
    return;
  }
  std::cout << computer.hostname;
  if (!computer.domain.empty()) {
    std::cout << '.' << computer.domain;
  }
  std::cout << std::endl;
  for (const auto& [name, iface] : computer.interfaces) {
    bool show_intf{};
    // Determine if this is a loop back device only
    for (const auto& address : iface.ipv4) {
      if (address.compare(0, 4, "127.") == 0) continue;
      show_intf = true;
    }
    if (!show_intf) continue;

    std::cout << "  " << name;
    if (!iface.mac.empty()) {
      std::cout << " mac=" << iface.mac;
    }
    if (iface.mtu != 0) {
      std::cout << " mtu=" << iface.mtu;
    }
    std::cout << std::endl;
    bool any_address{};
    for (const auto& address : iface.ipv4) {
      std::cout << (any_address ? ", " : "    ") << address;
      any_address = true;
    }
    if (any_address) std::cout << '\n';
  }
  std::cout << std::flush;
}

auto cli_presenter::on_terminated(int error) noexcept -> void {
  termination_error_ = error;
  // Process-directed, so it stays pending until sigwait() in run() takes it.
  kill(getpid(), SIGTERM);
}

}  // namespace sdd::cli
