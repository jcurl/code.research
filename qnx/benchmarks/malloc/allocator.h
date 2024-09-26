#ifndef BENCHMARK_ALLOCATOR_H
#define BENCHMARK_ALLOCATOR_H

#include <cstddef>

static constexpr std::size_t localmemsize = 8UL * 1024 * 1024;

auto get_allocated_localmem() -> std::size_t;

#endif