// #include <unistd.h>

#include <cstdint>
#include <cstring>
#include <iostream>

#include <benchmark/benchmark.h>

#include "options.h"

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::uint32_t buffer_size = 0;

__attribute__((noinline)) auto stride_copy(
    std::uint8_t* arr1, std::uint8_t* arr2, std::uint32_t slice) -> void {
  for (std::uint32_t s = 0; s < slice; s++) {
    for (std::uint32_t i = 0; i < buffer_size; i += slice) {
      // We know the copy is copying garbage data. We don't do anything with it.
      // It's for measuring the time of the operation only.
      //
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,clang-analyzer-core.uninitialized.Assign)
      arr2[i] = arr1[i];
    }
  }
}

__attribute__((noinline)) static void BM_CopyStride(benchmark::State& state) {
  int slice = static_cast<int>(state.range(0));

  // Using `std::vector` adds noise due to the `std::memset` and the data is not
  // so easy to read once plotted. Using the `new` operator doesn't clear the
  // memory first, with the test slightly faster and clearer to interpret the
  // results.

  // auto vec1 = std::vector<std::uint8_t>(size, 0);
  // auto vec2 = std::vector<std::uint8_t>(size, 0);
  // std::uint8_t *arr1 = vec1.data();
  // std::uint8_t *arr2 = vec2.data();

  // NOLINTBEGIN(cppcoreguidelines-owning-memory)
  auto arr1 = new std::uint8_t[buffer_size];
  auto arr2 = new std::uint8_t[buffer_size];
  // NOLINTEND(cppcoreguidelines-owning-memory)

  // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
  for (auto _ : state) {
    // This code gets timed
    stride_copy(arr1, arr2, slice);
  }

  // NOLINTBEGIN(cppcoreguidelines-owning-memory)
  delete[] arr1;
  delete[] arr2;
  // NOLINTEND(cppcoreguidelines-owning-memory)
}

// NOLINTBEGIN

// Register the function as a benchmark
BENCHMARK(BM_CopyStride)->DenseRange(16, 512, 1);

auto main(int argc, char** argv) -> int {
  benchmark::Initialize(&argc, argv);
  auto options = make_options(argc, argv);
  if (!options) return options.error();

  buffer_size = options->buffer_size() << 20;
  std::cout << "Using buffer size of: " << options->buffer_size() << "MB"
            << std::endl;
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}

// NOLINTEND
