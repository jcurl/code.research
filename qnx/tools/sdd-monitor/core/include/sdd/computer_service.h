#ifndef SDD_CORE_COMPUTER_SERVICE_H
#define SDD_CORE_COMPUTER_SERVICE_H

#include "sdd/computer_observer.h"
#include "sdd/subscription.h"
#include "stdext/expected.h"

namespace sdd::core {

/// @brief Interface to subscribe for notifications.
class computer_service {
 public:
  computer_service() = default;
  computer_service(const computer_service&) = delete;
  auto operator=(const computer_service&) -> computer_service& = delete;
  computer_service(computer_service&&) noexcept = delete;
  auto operator=(computer_service&&) noexcept -> computer_service& = delete;
  virtual ~computer_service() = default;

  /// @brief Subscribe to the service for notification updates.
  /// @param observer The object that that receives the notification updates
  /// @return A reference to a subscription, that when destroyed, unsubscribes.
  [[nodiscard]] virtual auto subscribe(computer_observer& observer)
      -> infra::subscription = 0;

  /// @brief Reports whether the service is currently receiving.
  /// @return No value while running. Otherwise the error that stopped the
  /// service: 0 when stopped on request, or the errno.
  [[nodiscard]] virtual auto is_running() const
      -> stdext::expected<void, int> = 0;
};

}  // namespace sdd::core

#endif
