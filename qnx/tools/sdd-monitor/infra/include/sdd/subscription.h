#ifndef SDD_INFRA_SUBSCRIPTIOIN_H
#define SDD_INFRA_SUBSCRIPTIOIN_H

#include <functional>
#include <utility>

/// @brief A subscription object that notifies when it is destroyed.
///
/// A class can do some work, and return a subscription object to its caller, so
/// that when the caller goes out of scope, the subscribed object is notified.
/// This allows the subscribed object to remove references to the object going
/// out of scope avoiding possible use after free.
class subscription {
 public:
  subscription() noexcept = default;

  /// @brief Subscribe constructor.
  /// @param cancel The function that should be called when this object goes out
  /// of scope. This function must not raise exceptions, else your program will
  /// terminate when this object is destroyed.
  explicit subscription(std::function<void()> cancel)
      : cancel_(std::move(cancel)) {}

  /// @brief Move constructor.
  /// @param o The object to move into.
  subscription(subscription&& o) noexcept
      : cancel_(std::exchange(o.cancel_, nullptr)) {}

  /// @brief Move assignment.
  ///
  /// If the object being moved into already has a cancel function, it will be
  /// called during the move.
  ///
  /// @param o The object to move into this.
  /// @return This object.
  auto operator=(subscription&& o) noexcept -> subscription& {
    subscription tmp(std::move(o));
    std::swap(cancel_, tmp.cancel_);
    return *this;
  }

  subscription(const subscription&) = delete;
  auto operator=(const subscription&) -> subscription& = delete;

  /// @brief The destructor
  ///
  /// Calls the cancel function given in the constructor when this object is
  /// destroyed, allowing the object returning the subscription object to be
  /// notified.
  ///
  /// It is important that the object being subscribed to has scope beyond this
  /// constructor, else the cancel function may reference stale information
  /// resulting in a use after free scenario.
  ~subscription() {
    if (cancel_) cancel_();
  }

 private:
  std::function<void()> cancel_{};
};

#endif