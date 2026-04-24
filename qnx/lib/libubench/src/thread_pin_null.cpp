#include <cerrno>

#include "ubench/thread.h"

namespace ubench::thread {

class pin_core::pin_core_impl {
 public:
  pin_core_impl(unsigned int core) : core_{core} {}
  pin_core_impl(const pin_core_impl&) = delete;
  pin_core_impl(pin_core_impl&&) = default;
  auto operator=(const pin_core_impl&) -> pin_core_impl& = delete;
  auto operator=(pin_core_impl&&) -> pin_core_impl& = default;
  ~pin_core_impl() = default;

  [[nodiscard]] auto core() const -> unsigned int { return core_; }

 private:
  unsigned int core_{};
};

pin_core::pin_core([[maybe_unused]] unsigned int core)
    : impl_{std::make_unique<pin_core::pin_core_impl>(core)} {}

pin_core::~pin_core() = default;

pin_core::operator bool() const { return false; }

auto pin_core::error() const -> int { return ENOSYS; }

auto pin_core::core() const -> unsigned int { return impl_->core(); }

pin_core::pin_core(pin_core&&) noexcept = default;

auto pin_core::operator=(pin_core&&) noexcept -> pin_core& = default;

}  // namespace ubench::thread
