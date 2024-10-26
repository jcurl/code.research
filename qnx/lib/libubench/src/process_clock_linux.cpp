#include <ctime>

#include "ubench/clock.h"

namespace ubench::chrono {

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
clockid_t clock_id = 0;

auto init_clock() -> bool {
  // We could use a constant, but documentation states we should do this
  // anyway to determine if the clock is supported on the system.
  int result = clock_getcpuclockid(0, &clock_id);
  return result == 0;
}

auto init_static() -> bool {
  static const auto result = init_clock();
  return result;
}

}  // namespace

auto process_clock::now() noexcept -> time_point {
  if (!init_static()) return time_point{duration{0}};

  struct timespec ts {};
  if (clock_gettime(clock_id, &ts)) return time_point{duration{0}};

  std::uint64_t tns = ts.tv_sec * 1000000000 + ts.tv_nsec;
  return time_point{
      std::chrono::duration_cast<duration>(std::chrono::nanoseconds(tns))};
}

auto process_clock::is_available() noexcept -> bool { return init_static(); }

}  // namespace ubench::chrono
