#include "ubench/measure/busy_measurement.h"

#include <thread>

namespace ubench::measure {

busy_stop_watch::busy_stop_watch() noexcept { reset(); }

auto busy_stop_watch::reset() -> void {
  start_idle_ = ubench::chrono::idle_clock::now();
  start_proc_ = ubench::chrono::process_clock::now();
  start_time_ = std::chrono::high_resolution_clock::now();
}

auto busy_stop_watch::measure() const noexcept -> busy_measurement {
  busy_measurement result{};
  auto end_idle = ubench::chrono::idle_clock::now();
  auto end_proc = ubench::chrono::process_clock::now();
  auto end_time = std::chrono::high_resolution_clock::now();

  // Use default nanosecond resolution to reduce rounding errors later when
  // multiplying by the number of cores.
  auto time_span = std::chrono::duration_cast<std::chrono::nanoseconds>(
      end_time - start_time_);
  auto idle_span = std::chrono::duration_cast<std::chrono::nanoseconds>(
      end_idle - start_idle_);
  auto proc_span = std::chrono::duration_cast<std::chrono::nanoseconds>(
      end_proc - start_proc_);
  auto cores = std::thread::hardware_concurrency();

  // For systems with almost no CPU busy, there could be a minor error in the
  // idle time and the running time of probably less than 200us.
  result.idle_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(idle_span);
  if (time_span * cores > idle_span) {
    result.busy_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        cores * time_span - idle_span);
  } else {
    result.busy_time = std::chrono::milliseconds(0);
  }

  result.cpu_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(proc_span);
  result.run_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(time_span);
  return result;
}

}  // namespace ubench::measure
