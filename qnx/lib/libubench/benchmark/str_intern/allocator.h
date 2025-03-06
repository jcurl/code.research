#ifndef BENCHMARK_ALLOCATOR_H
#define BENCHMARK_ALLOCATOR_H

#include <cstdint>

struct mem_metrics {
  std::int64_t total_alloc{};    //< Total bytes requested for allocation.
  std::int64_t total_free{};     //< Total bytes requested for free.
  std::int64_t max_alloc{};      //< Maximum bytes at a time allocated.
  std::int64_t current_alloc{};  //< Current bytes actually allocated.
  std::int64_t allocs{};
  std::int64_t frees{};
};

/// @brief Reset statistics.
///
/// Reset allocation statistics to zero. Some resultant statistics afterwards
/// may be negative!.
auto reset_alloc() -> void;

/// @brief Get a copy of the statistics.
///
/// A copy is made that is a snapshot at the time it is measured.
///
/// @return The statistics at the time of the call.
auto get_stats() -> mem_metrics;

#endif
