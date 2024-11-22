#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <benchmark/benchmark.h>

#include "allocator.h"
#include "mallopt.h"
#include "syspage.h"

// NOLINTBEGIN

static void BM_MallocFreeBench(benchmark::State& state) {
  // Perform setup here
  std::size_t alloc_size = state.range(0);

  for (auto _ : state) {
    // This code gets timed
    void* p = malloc(alloc_size);
    if (p) free(p);
  }
}

static void BM_CallocFreeBench(benchmark::State& state) {
  // Perform setup here
  std::size_t alloc_size = state.range(0);

  for (auto _ : state) {
    // This code gets timed
    void* p = calloc(alloc_size >> 4, 16);
    if (p) free(p);
  }
}

static void BM_MallocBench(benchmark::State& state) {
  // Perform setup here
  std::size_t alloc_size = state.range(0);

  for (auto _ : state) {
    // This code gets timed
    void* p = malloc(alloc_size);
    state.PauseTiming();
    if (p) free(p);
    state.ResumeTiming();
  }
}

static void BM_CallocBench(benchmark::State& state) {
  // Perform setup here
  std::size_t alloc_size = state.range(0);

  for (auto _ : state) {
    // This code gets timed
    void* p = calloc(alloc_size >> 4, 16);
    state.PauseTiming();
    if (p) free(p);
    state.ResumeTiming();
  }
}

static void BM_MFreeBench(benchmark::State& state) {
  // Perform setup here
  std::size_t alloc_size = state.range(0);

  for (auto _ : state) {
    // This code gets timed
    state.PauseTiming();
    void* p = malloc(alloc_size);
    state.ResumeTiming();
    if (p) free(p);
  }
}

static void BM_CFreeBench(benchmark::State& state) {
  // Perform setup here
  std::size_t alloc_size = state.range(0);

  for (auto _ : state) {
    // This code gets timed
    state.PauseTiming();
    void* p = calloc(alloc_size >> 4, 16);
    state.ResumeTiming();
    if (p) free(p);
  }
}

static void BM_MallocWalkFreeBench(benchmark::State& state) {
  // Perform setup here
  std::size_t alloc_size = state.range(0);

  for (auto _ : state) {
    // This code gets timed
    auto p = static_cast<std::uint8_t*>(malloc(alloc_size));
    if (p) {
      auto page_size = get_syspage_size();
      for (std::size_t i = 0; i < alloc_size; i += page_size) {
        p[i] = 0;
      }
      free(p);
    }
  }
}

static void BM_MallocClearFreeBench(benchmark::State& state) {
  // Perform setup here
  std::size_t alloc_size = state.range(0);

  for (auto _ : state) {
    // This code gets timed
    auto p = static_cast<std::uint8_t*>(malloc(alloc_size));
    if (p) {
      memset(p, 0, alloc_size);
      free(p);
    }
  }
}

static void BM_MallocClearWalkFreeBench(benchmark::State& state) {
  // Perform setup here
  std::size_t alloc_size = state.range(0);

  for (auto _ : state) {
    // This code gets timed
    auto p = static_cast<std::uint8_t*>(malloc(alloc_size));
    if (p) {
      memset(p, 0, alloc_size);
      auto page_size = get_syspage_size();
      for (std::size_t i = 0; i < alloc_size; i += page_size) {
        p[i] = 0;
      }
      free(p);
    }
  }
}

// Entry to benchmark, that parses the options to modify behaviour of what is
// being benchmarked.

auto main(int argc, char** argv) -> int {
  // By doing the registration here, we'll use our overrides for new/delete for
  // deterministic memory allocation (rather than it being done by the static
  // initialiser).
  benchmark::RegisterBenchmark("BM_MallocFreeBench", BM_MallocFreeBench)
      ->RangeMultiplier(2)
      ->Range(4096, 1 << 30);
  benchmark::RegisterBenchmark("BM_MallocBench", BM_MallocBench)
      ->RangeMultiplier(2)
      ->Range(4096, 1 << 30);
  benchmark::RegisterBenchmark("BM_MFreeBench", BM_MFreeBench)
      ->RangeMultiplier(2)
      ->Range(4096, 1 << 30);
  benchmark::RegisterBenchmark("BM_CallocFreeBench", BM_CallocFreeBench)
      ->RangeMultiplier(2)
      ->Range(4096, 1 << 30);
  benchmark::RegisterBenchmark("BM_CallocBench", BM_CallocBench)
      ->RangeMultiplier(2)
      ->Range(4096, 1 << 30);
  benchmark::RegisterBenchmark("BM_CFreeBench", BM_CFreeBench)
      ->RangeMultiplier(2)
      ->Range(4096, 1 << 30);
  benchmark::RegisterBenchmark("BM_MallocWalkFreeBench", BM_MallocWalkFreeBench)
      ->RangeMultiplier(2)
      ->Range(4096, 1 << 30);
  benchmark::RegisterBenchmark(
      "BM_MallocClearFreeBench", BM_MallocClearFreeBench)
      ->RangeMultiplier(2)
      ->Range(4096, 1 << 30);
  benchmark::RegisterBenchmark(
      "BM_MallocClearWalkFreeBench", BM_MallocClearWalkFreeBench)
      ->RangeMultiplier(2)
      ->Range(4096, 1 << 30);
  benchmark::Initialize(&argc, argv);

  // User specific options. All google-benchmark options have been stripped.
  mallopt_options options(argc, argv);
  if (!options.do_run()) return options.result();

  std::cout << "System Page Size: " << get_syspage_size() << std::endl;

  std::size_t init_alloc = get_allocated_localmem();
  benchmark::RunSpecifiedBenchmarks();
  std::size_t final_alloc = get_allocated_localmem();

  std::cout << "Used total local memory " << final_alloc << " of "
            << localmemsize << " bytes." << std::endl;
  std::cout << "Used by benchmarks " << (final_alloc - init_alloc) << std::endl;
  benchmark::Shutdown();
  return 0;
}

// NOLINTEND
