#ifndef SDD_CORE_LOCAL_COMPUTER_SERVICE_H
#define SDD_CORE_LOCAL_COMPUTER_SERVICE_H

#include <atomic>
#include <thread>
#include <vector>

#include "sdd/computer_observer.h"
#include "sdd/computer_service.h"
#include "sdd/local_computer_service_config.h"
#include "sdd/notifier.h"
#include "ubench/net.h"

namespace sdd::core {

/// @brief The local computer service receiving network packets.
class local_computer_service : public computer_service {
 public:
  /// @brief Constructor.
  /// @param config The multicast group and port to receive packets on.
  explicit local_computer_service(local_computer_service_config config);

  local_computer_service(const local_computer_service&) = delete;
  auto operator=(const local_computer_service&)
      -> local_computer_service& = delete;
  local_computer_service(local_computer_service&&) noexcept = delete;
  auto operator=(local_computer_service&&) noexcept
      -> local_computer_service& = delete;

  /// @brief Stops the receive thread and waits for it to exit.
  ~local_computer_service() override;

  /// @brief Subscribe to the service for notification updates.
  /// @param observer The object that that receives the notification updates
  /// @return A reference to a subscription, that when destroyed, unsubscribes.
  [[nodiscard]] auto subscribe(computer_observer& observer)
      -> infra::subscription override {
    return observers_.subscribe(observer);
  }

  /// @brief Opens the socket and starts the receive thread.
  /// @return The errno if the socket could not be set up.
  [[nodiscard]] auto start() -> stdext::expected<void, int>;

  /// @brief Requests the receive thread to exit. Returns without waiting.
  auto stop() noexcept -> void { stop_requested_ = true; }

  /// @brief Reports whether the receive thread is running.
  /// @return No value while running. Otherwise the error that stopped it: 0
  /// when stopped on request, or the errno that ended the thread or stopped
  /// start().
  [[nodiscard]] auto is_running() const noexcept
      -> stdext::expected<void, int> override {
    if (running_) return {};
    return stdext::unexpected{last_error_.load()};
  }

  /// @brief Call to update in the local datastructures and notify all
  /// subscribers.
  /// @param packet The packet received
  /// @return The computer identifier associated with the packet.
  auto computer_ping_received(const computer& packet) -> int;

 private:
  auto open_socket() -> stdext::expected<ubench::net::udp, int>;
  auto run(ubench::net::udp socket) -> void;

  local_computer_service_config config_;
  infra::notifier<computer_observer> observers_{};
  std::vector<computer> computers_{};
  std::atomic<bool> stop_requested_{false};
  std::atomic<bool> running_{false};
  std::atomic<int> last_error_{0};

  // Declared last so that it is joined before the members it uses are
  // destroyed.
  std::thread worker_{};
};

}  // namespace sdd::core

#endif
