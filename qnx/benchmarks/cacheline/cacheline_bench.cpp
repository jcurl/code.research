#include <unistd.h>

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include <benchmark/benchmark.h>

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

auto print_help(std::string_view prog_name) -> void {
  std::cout << "USAGE: " << prog_name << " [-b <buffer>]" << std::endl;
  std::cout << std::endl;
  std::cout << "Execute strided copy test for <buffer> MB (default is 256MB)."
            << std::endl;

  std::cout << std::endl;
  benchmark::PrintDefaultHelp();
}

auto main(int argc, char** argv) -> int {
  benchmark::Initialize(&argc, argv);

  // User specific options. All google-benchmark options have been stripped.
  bool help = false;
  int c = 0;
  int exit_code = 0;
  std::uint32_t buffer_size_opt = 256;

  while ((c = getopt(argc, argv, "b:?")) != -1) {
    switch (c) {
      case 'b': {
        std::string buffer_arg = optarg;
        auto [ptr, ec] = std::from_chars(buffer_arg.data(),
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            buffer_arg.data() + buffer_arg.size(), buffer_size_opt);
        if (ec == std::errc{}) {
          if (buffer_size_opt < 1 || buffer_size_opt > 512) {
            std::cerr
                << "Error: Buffer size should be from 1 to 512 (units of MB)"
                << std::endl;
            exit_code = 1;
            help = true;
          }
        } else {
          std::cerr << "Error: Specify a buffer sizse as a number" << std::endl;
          exit_code = 1;
          help = true;
        }
        break;
      }
      case '?':
        help = true;
        if (optopt) exit_code = 1;
        break;
      case ':':
        std::cerr << "Error: Option -" << optopt << " requires an operand"
                  << std::endl;
        exit_code = 1;
        help = true;
        break;
      default:
        std::cerr << "Error: Unknown option -" << optopt << std::endl;
        exit_code = 1;
        help = true;
        break;
    }
  }

  if (help) {
    if (exit_code) std::cerr << std::endl;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    print_help(std::string_view(argv[0]));
    return exit_code;
  }

  buffer_size = buffer_size_opt * 1 << 20;
  std::cout << "Using buffer size of: " << buffer_size_opt << std::endl;
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}

// NOLINTEND
