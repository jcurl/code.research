#include "config.h"

#include <pthread.h>
#include <sched.h>

#if HAVE_INCLUDE_PTHREAD_NP_H
// FreeBSD
#include <pthread_np.h>
#endif

#include <cerrno>
#include <thread>

#include "ubench/thread.h"

namespace ubench::thread {

class pin_core::pin_core_impl {
 public:
  pin_core_impl(unsigned int core) : core_{core} {
    if (core >= ubench::thread::thread_count()) return;

    thread_ = pthread_self();

    int rc = 0;
    CPU_ZERO(&orig_mask_);
    rc = pthread_getaffinity_np(thread_, sizeof(cpu_set_t), &orig_mask_);
    if (rc) {
      error_ = errno;
      return;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);
    rc = pthread_setaffinity_np(thread_, sizeof(cpu_set_t), &cpuset);
    if (rc) {
      error_ = errno;
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
    pthread_setaffinity_np(thread_, sizeof(cpu_set_t), &orig_mask_);
  }

  [[nodiscard]] auto is_pinned() const -> bool { return pinned_; }

  [[nodiscard]] auto error() const -> int { return error_; }

  [[nodiscard]] auto core() const -> unsigned int { return core_; }

 private:
  unsigned int core_{};
  bool pinned_{};
  int error_{};
  pthread_t thread_;
  cpu_set_t orig_mask_{};
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
