#include <sys/neutrino.h>

#include <cinttypes>

#include "ubench/clock.h"

namespace ubench::chrono {

auto process_clock::now() noexcept -> time_point {
  std::uint64_t time = 0;
  int r = ClockTime(CLOCK_PROCESS_CPUTIME_ID, nullptr, &time);
  if (r == -1) return time_point{duration(0)};

  // QNX provides the times in nano-seconds.
  return time_point{
      std::chrono::duration_cast<duration>(std::chrono::nanoseconds(time))};
}

auto process_clock::is_available() noexcept -> bool { return true; }

}  // namespace ubench::chrono
