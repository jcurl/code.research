#ifndef UBENCH_MEASUREMENT_BUSY_MEASUREMENT_H
#define UBENCH_MEASUREMENT_BUSY_MEASUREMENT_H

#include <chrono>

#include "ubench/clock.h"

namespace ubench::measure {

/// @brief A measurement of CPU clocks.
struct busy_measurement {
  std::chrono::milliseconds idle_time;  //< Idle time summed over all CPUs.
  std::chrono::milliseconds
      busy_time;  //< System CPU usage time summed over all CPUs.
  std::chrono::milliseconds
      cpu_time;  //< Process CPU usage time summed over all CPUs.
  std::chrono::milliseconds run_time;  //< Wall-clock time
};

/// @brief A stop watch for busy measurements.
///
/// Construct the option which defines the start point when timing begins. Use
/// measure() to get the times elapsed (idle, cpu and elapsed).
class busy_stop_watch {
 public:
  /// @brief Initiate the watchdog by capturing time stamps.
  explicit busy_stop_watch() noexcept;

  /// @brief Reset the timestamps as if the object were constructed.
  auto reset() -> void;

  /// @brief Get the time the object was last reset.
  ///
  /// @return the time the object was last reset.
  [[nodiscard]] auto start_time() const noexcept
      -> std::chrono::high_resolution_clock::time_point {
    return start_time_;
  }

  /// @brief Get the time elapsed since the last reset.
  ///
  /// @return the time elapsed since the last reset.
  [[nodiscard]] auto elapsed_time() const noexcept
      -> std::chrono::milliseconds {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time_);
  }

  /// @brief Make a time difference measurement.
  ///
  /// @return a structure giving the idle, cpu and elapsed time since reset.
  [[nodiscard]] auto measure() const noexcept -> busy_measurement;

 private:
  ubench::chrono::idle_clock::time_point start_idle_{};
  ubench::chrono::process_clock::time_point start_proc_{};
  std::chrono::high_resolution_clock::time_point start_time_{};
};

}  // namespace ubench::measure

#endif
