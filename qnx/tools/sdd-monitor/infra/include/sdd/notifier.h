#ifndef SDD_INFRA_NOTIFIER_H
#define SDD_INFRA_NOTIFIER_H

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <mutex>
#include <vector>

#include "sdd/subscription.h"

namespace sdd::infra {

/// @brief Handle commonly notification operations using subscribers.
///
/// There are two objects, the client C which is subscribing for notifications,
/// and S, the server which notifies the subscriber C when notify() is called.
/// The client keeps a copy of the subscription, so when it goes out of scope,
/// the subscription is destroyed, thus informing the notifier, S. The notifier
/// S, maintains an object of this class.
///
/// @tparam L The type of listener (interface describing how to notify)
template <typename L>
class notifier {
 public:
  /// @brief Default constructor.
  notifier() { listeners_.reserve(8); }

  /// @brief Subscribe to this notifier, S.
  /// @param l The object C, that is subscribing to this notifier, S.
  /// @return A subscription, that the notified object C should keep, and only
  /// free when it is out of scope. The notified object C should not outlive
  /// this notifier S.
  [[nodiscard]] auto subscribe(L& l) -> subscription {
    {
      std::scoped_lock lock(mutex_);
      listeners_.push_back(&l);
    }
    return subscription([this, &l]() {
      {
        std::scoped_lock lock(mutex_);
        listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), &l),
            listeners_.end());
      }
      std::unique_lock lock(mutex_);
      cv_.wait(lock, [this] { return in_flight_ == 0; });
    });
  }

  /// @brief The object being subscribed to, S,
  /// @tparam C The object that knows how to invoke the listener L.
  /// @param call The reference to the object that knows how to invoke the
  /// listener, L.
  /// @param rethrow If true, the first exception raised by a listener is
  /// rethrown after all listeners have been notified. Otherwise it is
  /// discarded.
  template <typename C>
  auto notify(C&& call, bool rethrow = false) -> void {
    std::vector<L*> snapshot;
    {
      std::scoped_lock lock(mutex_);
      snapshot = {listeners_.begin(), listeners_.end()};
      // Must be after making the snapshot, so if it throws, we don't deadlock.
      ++in_flight_;
    }

    std::exception_ptr first_error;
    for (L* l : snapshot) {
      try {
        call(*l);
      } catch (...) {
        if (!first_error) first_error = std::current_exception();
      }
    }

    {
      std::scoped_lock lock(mutex_);
      --in_flight_;
    }
    cv_.notify_all();

    // Ahem. Callbacks caused a problem.
    if (rethrow && first_error) std::rethrow_exception(first_error);
  }

 private:
  std::mutex mutex_{};
  std::condition_variable cv_{};
  std::vector<L*> listeners_{};
  std::size_t in_flight_{0};
};

}  // namespace sdd::infra

#endif
