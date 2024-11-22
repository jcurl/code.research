#include "ubench/clock.h"

namespace ubench::chrono {

auto idle_clock::now() noexcept -> time_point {
  return time_point{duration(0)};
}

auto idle_clock::is_available() noexcept -> bool { return false; }

auto idle_clock::type() noexcept -> idle_clock_type {
  return idle_clock_type::null;
}

}  // namespace ubench::chrono