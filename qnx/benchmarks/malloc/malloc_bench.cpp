#include "config.h"

#include <unistd.h>
#if HAVE_MALLOPT
#include <malloc.h>
#endif

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <benchmark/benchmark.h>

#include "ubench/os.h"
#include "ubench/string.h"
#include "allocator.h"
#include "mallopt.h"
#include "mlock.h"

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

  auto page_size = ubench::os::get_syspage_size();
  if (!page_size) {
    std::cerr << "Can't get the system page size: "
              << ubench::string::perror(page_size.error());
    return;
  }
  for (auto _ : state) {
    // This code gets timed
    auto p = static_cast<std::uint8_t*>(malloc(alloc_size));
    if (p) {
      for (std::size_t i = 0; i < alloc_size; i += *page_size) {
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
  auto page_size = ubench::os::get_syspage_size();
  if (!page_size) {
    std::cerr << "Can't get the system page size: "
              << ubench::string::perror(page_size.error());
    return;
  }

  for (auto _ : state) {
    // This code gets timed
    auto p = static_cast<std::uint8_t*>(malloc(alloc_size));
    if (p) {
      memset(p, 0, alloc_size);
      for (std::size_t i = 0; i < alloc_size; i += *page_size) {
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
  auto options = make_options(argc, argv);
  if (!options) return options.error();

  if (options->mlock_all()) {
    auto success = enable_mlockall();
    if (!success) {
      std::cout << "Error locking with -L, "
                << ubench::string::perror(success.error()) << std::endl;
      return 1;
    } else {
      std::cout << "Memory Locking ENABLED" << std::endl;
    }
  }

#if HAVE_MALLOPT
  if (!options->mallopts().empty()) {
    for (const auto& opt : options->mallopts()) {
      int result = mallopt(std::get<0>(opt), std::get<1>(opt));
#ifdef __QNX__
      // Under QNX: 0 on success, or -1 if an error occurs (errno is set).
      if (std::get<0>(opt) == MALLOC_ARENA_SIZE) {
        // For `MALLOC_ARENA_SIZE`, if value is 0, `mallopt()` returns the
        // current arena size; if value is any other value, `mallopt()` returns
        // the previous size.
        result = 0;
      }
      if (result) {
        if (errno) {
          std::cout << ubench::string::perror(errno) << std::endl;
        }
        std::cout << "mallopt(" << std::get<2>(opt) << ", " << std::get<1>(opt)
                  << ")"
                  << " returns " << result << ". Continuing." << std::endl;
      }
#else
      // Under Linux: On success, mallopt() returns 1.  On error, it returns 0.
      if (!result) {
        if (errno) {
          std::cout << ubench::string::perror(errno) << std::endl;
        }
        std::cout << "mallopt(" << std::get<2>(opt) << ", " << std::get<1>(opt)
                  << ")"
                  << " returns " << result << ". Continuing." << std::endl;
      }
#endif
      else {
        std::cout << "mallopt(" << std::get<2>(opt) << ", " << std::get<1>(opt)
                  << ") is set." << std::endl;
      }
    }
  }
#endif

  auto page_size = ubench::os::get_syspage_size();
  if (!page_size) {
    std::cout << "System Page Size: UNKNOWN" << std::endl;
  } else {
    std::cout << "System Page Size: " << *page_size << std::endl;
  }

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
