#include <cstdlib>

#include <benchmark/benchmark.h>

#include "ubench/string.h"

static const std::string byte_string{"12"};
static const std::string short_string{"fa23"};
static const std::string long_string{"de45ca28"};
static const std::string llong_string{"87ea24c26e7b3d21"};
static const std::string xlong_string{"26ef1ac49067fe375e2b1ac98c58f10d"};

static __attribute__((noinline)) auto test_strtol(const std::string& str)
    -> std::int64_t {
  char* endptr = nullptr;
  return strtol(str.c_str(), &endptr, 16);
}

static __attribute__((noinline)) auto test_strtoll(const std::string& str)
    -> std::int64_t {
  char* endptr = nullptr;
  return strtoll(str.c_str(), &endptr, 16);
}

static __attribute__((noinline)) auto test_strtoul(const std::string& str)
    -> std::uint64_t {
  char* endptr = nullptr;
  return strtoul(str.c_str(), &endptr, 16);
}

static __attribute__((noinline)) auto test_strtoull(const std::string& str)
    -> std::uint64_t {
  char* endptr = nullptr;
  return strtoull(str.c_str(), &endptr, 16);
}

template <typename T>
static __attribute__((noinline)) auto test_from_chars_hex(
    const std::string& str) -> T {
  T value{};
  /* auto [ptr, ec] =*/ubench::string::from_chars_hex(
      // NOLINTNEXTLINE(clang-diagnostic-unused-variable,cppcoreguidelines-pro-bounds-pointer-arithmetic)
      str.data(), str.data() + str.size(), value);
  return value;
}

static __attribute__((noinline)) auto test_from_chars(const std::string& str)
    -> std::uint64_t {
  std::uint64_t value{};
  /* auto [ptr, ec] =*/std::from_chars(
      // NOLINTNEXTLINE(clang-diagnostic-unused-variable,cppcoreguidelines-pro-bounds-pointer-arithmetic)
      str.data(), str.data() + str.size(), value, 16);
  return value;
}

// NOLINTBEGIN

static void BM_StrToL_byte(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtol(byte_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToL_short(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtol(short_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToL_long(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtol(long_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToL_llong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtol(llong_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToL_xlong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtol(xlong_string);
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_StrToL_byte);
BENCHMARK(BM_StrToL_short);
BENCHMARK(BM_StrToL_long);
BENCHMARK(BM_StrToL_llong);
BENCHMARK(BM_StrToL_xlong);

static void BM_StrToLL_byte(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoll(byte_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToLL_short(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoll(short_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToLL_long(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoll(long_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToLL_llong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoll(llong_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToLL_xlong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoll(xlong_string);
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_StrToLL_byte);
BENCHMARK(BM_StrToLL_short);
BENCHMARK(BM_StrToLL_long);
BENCHMARK(BM_StrToLL_llong);
BENCHMARK(BM_StrToLL_xlong);

static void BM_StrToUL_byte(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoul(byte_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToUL_short(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoul(short_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToUL_long(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoul(long_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToUL_llong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoul(llong_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToUL_xlong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoul(xlong_string);
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_StrToUL_byte);
BENCHMARK(BM_StrToUL_short);
BENCHMARK(BM_StrToUL_long);
BENCHMARK(BM_StrToUL_llong);
BENCHMARK(BM_StrToUL_xlong);

static void BM_StrToULL_byte(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoull(byte_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToULL_short(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoull(short_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToULL_long(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoull(long_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToULL_llong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoull(llong_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_StrToULL_xlong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_strtoull(xlong_string);
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_StrToULL_byte);
BENCHMARK(BM_StrToULL_short);
BENCHMARK(BM_StrToULL_long);
BENCHMARK(BM_StrToULL_llong);
BENCHMARK(BM_StrToULL_xlong);

static void BM_FromChars_byte(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars(byte_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_FromChars_short(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars(short_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_FromChars_long(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars(long_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_FromChars_llong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars(llong_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_FromChars_xlong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars(xlong_string);
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_FromChars_byte);
BENCHMARK(BM_FromChars_short);
BENCHMARK(BM_FromChars_long);
BENCHMARK(BM_FromChars_llong);
BENCHMARK(BM_FromChars_xlong);

static void BM_FromCharsHex_ubyte(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars_hex<std::uint8_t>(byte_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_FromCharsHex_ushort(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars_hex<std::uint16_t>(short_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_FromCharsHex_ulong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars_hex<std::uint32_t>(long_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_FromCharsHex_ullong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars_hex<std::uint64_t>(llong_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_FromCharsHex_uxlong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars_hex<std::uint64_t>(xlong_string);
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_FromCharsHex_ubyte);
BENCHMARK(BM_FromCharsHex_ushort);
BENCHMARK(BM_FromCharsHex_ulong);
BENCHMARK(BM_FromCharsHex_ullong);
BENCHMARK(BM_FromCharsHex_uxlong);

static void BM_FromCharsHex_byte(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars_hex<std::int8_t>(byte_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_FromCharsHex_short(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars_hex<std::int16_t>(short_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_FromCharsHex_long(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars_hex<std::int32_t>(long_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_FromCharsHex_llong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars_hex<std::int64_t>(llong_string);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_FromCharsHex_xlong(benchmark::State& state) {
  for (auto _ : state) {
    auto result = test_from_chars_hex<std::int64_t>(xlong_string);
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_FromCharsHex_byte);
BENCHMARK(BM_FromCharsHex_short);
BENCHMARK(BM_FromCharsHex_long);
BENCHMARK(BM_FromCharsHex_llong);
BENCHMARK(BM_FromCharsHex_xlong);

// Run the benchmark
BENCHMARK_MAIN();

// NOLINTEND
