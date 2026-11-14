#ifndef SDD_CORE_LOCAL_COMPUTER_SERVICE_H
#define SDD_CORE_LOCAL_COMPUTER_SERVICE_H

#include <vector>

#include "sdd/computer_observer.h"
#include "sdd/computer_service.h"
#include "sdd/notifier.h"

namespace sdd::core {

/// @brief The local computer service receiving network packets.
class local_computer_service : public computer_service {
 public:
  /// @brief Default constructor.
  local_computer_service() { computers_.reserve(16); }

  local_computer_service(const local_computer_service&) = delete;
  auto operator=(const local_computer_service&)
      -> local_computer_service& = delete;
  local_computer_service(local_computer_service&&) noexcept = delete;
  auto operator=(local_computer_service&&) noexcept
      -> local_computer_service& = delete;

  /// @brief Subscribe to the service for notification updates.
  /// @param observer The object that that receives the notification updates
  /// @return A reference to a subscription, that when destroyed, unsubscribes.
  [[nodiscard]] auto subscribe(computer_observer& observer)
      -> infra::subscription override {
    return observers_.subscribe(observer);
  }

  /// @brief Call to update in the local datastructures and notify all
  /// subscribers.
  /// @param packet The packet received
  /// @return The computer identifier associated with the packet.
  auto computer_ping_received(const computer& packet) -> int;

 private:
  infra::notifier<computer_observer> observers_{};
  std::vector<computer> computers_{};
};

}  // namespace sdd::core

#endif
