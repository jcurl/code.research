#include <sys/neutrino.h>

#include <cerrno>
#include <thread>

#include "ubench/thread.h"

namespace ubench::thread {

class pin_core::pin_core_impl {
 public:
  pin_core_impl(unsigned int core) : core_{core} {
    if (core >= ubench::thread::thread_count()) {
      errno_ = EINVAL;
      return;
    }

    self_ = pthread_self();

    runmask_ = 1 << core;
    int result = ThreadCtl(_NTO_TCTL_RUNMASK_GET_AND_SET, &runmask_);
    if (result == -1) {
      errno_ = errno;
      return;
    }
    pinned_ = true;
  }

  pin_core_impl(const pin_core_impl&) = delete;
  pin_core_impl(pin_core_impl&&) = default;
  auto operator=(const pin_core_impl&) -> pin_core_impl& = delete;
  auto operator=(pin_core_impl&&) -> pin_core_impl& = default;

  ~pin_core_impl() {
    if (!pinned_) return;
    ThreadCtl(_NTO_TCTL_RUNMASK_GET_AND_SET, &runmask_);
  }

  [[nodiscard]] auto is_pinned() const -> bool { return pinned_; }

  [[nodiscard]] auto core() const -> unsigned int { return core_; }

  [[nodiscard]] auto error() const -> int { return errno_; }

 private:
  unsigned int core_{};
  bool pinned_{};
  unsigned int runmask_{};
  pthread_t self_;
  int errno_{};
};

pin_core::pin_core([[maybe_unused]] unsigned int core)
    : impl_{std::make_unique<pin_core::pin_core_impl>(core)} {}

pin_core::~pin_core() = default;

pin_core::operator bool() const { return impl_->is_pinned(); }

auto pin_core::error() const -> int { return impl_->error(); }

auto pin_core::core() const -> unsigned int { return impl_->core(); }

pin_core::pin_core(pin_core&&) noexcept = default;

auto pin_core::operator=(pin_core&&) noexcept -> pin_core& = default;

}  // namespace ubench::thread
