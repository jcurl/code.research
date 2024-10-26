#include "ubench/clock.h"

namespace ubench::chrono {

auto process_clock::now() noexcept -> time_point {
  return time_point{std::chrono::milliseconds(0)};
}

auto process_clock::is_available() noexcept -> bool { return false; }

}  // namespace ubench::chrono