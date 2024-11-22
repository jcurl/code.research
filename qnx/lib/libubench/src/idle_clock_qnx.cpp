#include <sys/neutrino.h>
#include <inttypes.h>

#include <cerrno>
#include <mutex>
#include <thread>
#include <vector>

#include "ubench/clock.h"

namespace ubench::chrono {

namespace {

// Will contain the clock identifiers. Initialised on the first use.
std::vector<int> idle_clocks{};

auto init_idle_clocks() -> bool {
  auto threads = std::thread::hardware_concurrency();
  idle_clocks.reserve(threads);
  for (std::size_t i = 0; i < threads; i++) {
    int result = ClockId(1, i + 1);
    if (result == -1) {
      idle_clocks.clear();
      return false;
    }
    idle_clocks.emplace_back(result);
  }
  return true;
}

// Allow lazy initialisation of the static data.
auto init_static() -> bool {
  static const auto result = init_idle_clocks();
  return result;
}

}  // namespace

auto idle_clock::now() noexcept -> time_point {
  if (!init_static()) return time_point{duration(0)};

  std::uint64_t total_idle_time = 0;
  for (auto clock_id : idle_clocks) {
    std::uint64_t idle_time = 0;
    int r = ClockTime(clock_id, nullptr, &idle_time);
    if (r == -1) {
      return time_point{duration(0)};
    }
    total_idle_time += idle_time;
  }

  // QNX provides the times in nano-seconds.
  return time_point{std::chrono::duration_cast<duration>(
      std::chrono::nanoseconds(total_idle_time))};
}

auto idle_clock::is_available() noexcept -> bool { return init_static(); }

auto idle_clock::type() noexcept -> idle_clock_type {
  if (init_static()) return idle_clock_type::qnx;
  return idle_clock_type::null;
}

}  // namespace ubench::chrono
