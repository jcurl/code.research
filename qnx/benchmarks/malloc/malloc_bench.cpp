#include <benchmark/benchmark.h>

#include <cstddef>

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

static void BM_FreeBench(benchmark::State& state) {
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

// Register the function as a benchmark
BENCHMARK(BM_MallocFreeBench)->RangeMultiplier(2)->Range(16384, 1 << 30);
BENCHMARK(BM_MallocBench)->RangeMultiplier(2)->Range(16384, 1 << 30);
BENCHMARK(BM_FreeBench)->RangeMultiplier(2)->Range(16384, 1 << 30);

// Run the benchmark
BENCHMARK_MAIN();

// NOLINTEND
