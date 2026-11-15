#ifndef SDD_CLI_CLI_PRESENTER_H
#define SDD_CLI_CLI_PRESENTER_H

#include <atomic>
#include <ctime>
#include <map>
#include <mutex>
#include <string>

#include "sdd/computer_observer.h"
#include "sdd/computer_service.h"
#include "sdd/subscription.h"

namespace sdd::cli {

/// @brief Prints computer notifications to the console.
class cli_presenter : public core::computer_observer {
 public:
  /// @brief Subscribes to the service so computers are printed as received.
  /// @param service The service that publishes computer notifications.
  explicit cli_presenter(core::computer_service& service);

  cli_presenter(const cli_presenter&) = delete;
  auto operator=(const cli_presenter&) -> cli_presenter& = delete;
  cli_presenter(cli_presenter&&) noexcept = delete;
  auto operator=(cli_presenter&&) noexcept -> cli_presenter& = delete;

  /// @brief Waits for a signal, or for the service to terminate. While
  /// waiting, reports computers that have not been seen recently as lost.
  /// Signals must already be blocked in the calling thread and in the service
  /// receive thread, so they are consumed by sigtimedwait().
  /// @return The errno that stopped the service, or zero otherwise.
  [[nodiscard]] auto run() -> int;

  /// @brief Prints the computer when it is first seen, or seen again after it
  /// was reported lost. Later updates are not printed.
  /// @param computer The record that is new or has been updated.
  auto on_computer(const core::computer& computer) -> void override;

  /// @brief Called when the service receive thread exits. Nothing is printed,
  /// and run() is woken up.
  /// @param error The errno that stopped the thread, or zero if stopped on
  /// request.
  auto on_terminated(int error) noexcept -> void override;

 private:
  struct seen_computer {
    std::string name;
    std::time_t last_seen;
  };

  auto expire_lost_computers() -> void;

  std::atomic<int> termination_error_{0};
  std::mutex mutex_;
  std::map<int, seen_computer> seen_;
  infra::subscription subscription_;
};

}  // namespace sdd::cli

#endif
