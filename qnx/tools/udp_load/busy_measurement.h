#ifndef BENCHMARK_IDLE_MEASUREMENT_H
#define BENCHMARK_IDLE_MEASUREMENT_H

#include <chrono>

#include "ubench/clock.h"

struct busy_measurement {
  std::chrono::milliseconds idle_time;  //< Idle time summed over all CPUs.
  std::chrono::milliseconds
      busy_time;  //< System CPU usage time summed over all CPUs.
  std::chrono::milliseconds
      cpu_time;  //< Process CPU usage time summed over all CPUs.
  std::chrono::milliseconds run_time;  //< Wall-clock time
};

class busy_stop_watch {
 public:
  explicit busy_stop_watch() noexcept;
  auto start() const noexcept
      -> std::chrono::high_resolution_clock::time_point {
    return start_time_;
  }
  auto elapsed() const noexcept -> std::chrono::milliseconds {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time_);
  }
  auto measure() const noexcept -> busy_measurement;

 private:
  ubench::chrono::idle_clock::time_point start_idle_{};
  ubench::chrono::process_clock::time_point start_proc_{};
  std::chrono::high_resolution_clock::time_point start_time_{};
};

#endif
