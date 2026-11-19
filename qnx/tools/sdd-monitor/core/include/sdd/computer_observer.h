#ifndef SDD_CORE_COMPUTER_OBSERVER_H
#define SDD_CORE_COMPUTER_OBSERVER_H

#include "sdd/computer.h"

namespace sdd::core {

/// @brief Interface to implement for notifications of computer pings.
class computer_observer {
 public:
  computer_observer() = default;
  computer_observer(const computer_observer&) = delete;
  auto operator=(const computer_observer&) -> computer_observer& = delete;
  computer_observer(computer_observer&&) noexcept = delete;
  auto operator=(computer_observer&&) noexcept -> computer_observer& = delete;
  virtual ~computer_observer() = default;

  /// @brief Called when there is an update to a computer record.
  /// @param computer The record that is new or has been updated.
  virtual auto on_computer(const data::computer&) -> void = 0;

  /// @brief Called just before the service receive thread exits.
  /// @param error The errno that stopped the thread, or zero if stopped on
  /// request.
  virtual auto on_terminated(int error) -> void = 0;
};

}  // namespace sdd::core

#endif
