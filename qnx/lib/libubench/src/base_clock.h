#ifndef UBENCH_CHRONO_BASE_CLOCK_H
#define UBENCH_CHRONO_BASE_CLOCK_H

#include "ubench/clock.h"

namespace ubench::chrono {

class base_clock {
 public:
  base_clock() noexcept = default;
  base_clock(const base_clock& other) = delete;
  auto operator=(const base_clock& other) -> base_clock& = delete;
  base_clock(base_clock&& other) = delete;
  auto operator=(base_clock&& other) -> base_clock& = delete;
  virtual ~base_clock() = default;

  [[nodiscard]] virtual auto is_enabled() const noexcept -> bool {
    return false;
  }
  [[nodiscard]] virtual auto get_idle_clock() noexcept -> std::uint64_t {
    return 0;
  }
  [[nodiscard]] virtual auto type() const noexcept -> idle_clock_type {
    return idle_clock_type::null;
  }
};

}  // namespace ubench::chrono

#endif
