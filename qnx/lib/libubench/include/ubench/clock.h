#ifndef UBENCH_CHRONO_CLOCK_H
#define UBENCH_CHRONO_CLOCK_H

#include <chrono>

namespace ubench::chrono {

/// @brief Provides the clock type used to measure the idle time.
enum class idle_clock_type {
  null,       ///< Clock is non-functional.
  proc_stat,  ///< Clock uses /proc/stat and jiffies.
  qnx,        ///< Clock uses QNX ClockTime() call.
  windows,    ///< Clock uses Windows.
  cptime,     ///< Clock from sysctl(kern.cp_time)
};

/// @brief A TrivialClock that returns the total idle time for the Operating
/// System up until the point now().
struct idle_clock {
  using duration = std::chrono::nanoseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<idle_clock, duration>;
  static constexpr const bool is_steady = true;

  /// @brief The type of clock in use, that an application can get an idea of
  /// the usefulness of the clock.
  ///
  /// @return The clock type.
  static auto type() noexcept -> idle_clock_type;

  /// @brief Get the total idle time (of all CPUs) at the time of invocation.
  ///
  /// @return A time_point that can be used for comparison with other
  /// invocations of idle time.
  static auto now() noexcept -> time_point;

  /// @brief Test if the implementation provides meaningful results.
  ///
  /// @return true if implemented and can be used; false if the values always
  /// return zero idle time.
  static auto is_available() noexcept -> bool;
};

struct process_clock {
  using duration = std::chrono::nanoseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<process_clock, duration>;
  static constexpr const bool is_steady = true;

  /// @brief Get the total idle time (of all CPUs) at the time of invocation.
  ///
  /// @return A time_point that can be used for comparison with other
  /// invocations of idle time.
  static auto now() noexcept -> time_point;

  /// @brief Test if the implementation provides meaningful results.
  ///
  /// @return true if implemented and can be used; false if the values always
  /// return zero idle time.
  static auto is_available() noexcept -> bool;
};

}  // namespace ubench::chrono

#endif
