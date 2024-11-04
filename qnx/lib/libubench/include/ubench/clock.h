#ifndef UBENCH_IDLE_CLOCK_H
#define UBENCH_IDLE_CLOCK_H

#include <chrono>

namespace ubench::chrono {

/// @brief Provides the clock type used to measure the idle time.
enum class idle_clock_type {
  null,       ///< Clock is non-functional.
  proc_stat,  ///< Clock uses /proc/stat and jiffies.
  qnx,        ///< Clock uses QNX ClockTime() call.
  windows,    ///< Clock uses Windows.
};

/// @brief A TrivialClock that returns the total idle time for the Operating
/// System up until the point now().
struct idle_clock {
  typedef std::chrono::nanoseconds duration;
  typedef duration::rep rep;
  typedef duration::period period;
  typedef std::chrono::time_point<idle_clock, duration> time_point;
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
  typedef std::chrono::nanoseconds duration;
  typedef duration::rep rep;
  typedef duration::period period;
  typedef std::chrono::time_point<process_clock, duration> time_point;
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
