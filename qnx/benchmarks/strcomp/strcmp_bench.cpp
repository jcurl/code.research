#include <cstring>
#include <string>
#include <string_view>

#include <benchmark/benchmark.h>

#ifdef NDEBUG
#include <cassert>
#endif

// --------------------------------------------------------------------------
// std::string_view checks
// --------------------------------------------------------------------------

template <bool l = false>
static __attribute__((noinline)) auto StrFindWithSubStr(
    const std::string& str, const std::string& substr) -> bool {
  auto token_len = substr.length();
  if (l)
    if (str.length() < token_len) return false;
  return substr == str.substr(0, token_len);
}

template <bool l = false>
static __attribute__((noinline)) auto StrFindWithCompare(
    const std::string& str, const std::string& substr) -> bool {
  auto token_len = substr.length();
  if (l)
    if (str.length() < token_len) return false;
  return str.compare(0, token_len, substr) == 0;
}

template <bool l = false>
static __attribute__((noinline)) auto StrFindWithFirstOf(
    const std::string& str, const std::string& substr) -> bool {
  if (l) {
    auto token_len = substr.length();
    if (str.length() < token_len) return false;
  }
  return str.find_first_of(substr, 0) == 0;
}

// NOLINTBEGIN

static std::string initial_string{"lighttpd_server.246"};
static std::string token_match{"lighttpd_server."};
static std::string token_nomatch{"systemd."};
static std::string token_nomatchlonger{
    "this_is_a_longer_string_that_doesnt_match"};

static void BM_StrFindWithSubStr(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithSubStr(initial_string, token_match);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(result);
#endif
  }
}

static void BM_StrFindWithSubStrLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithSubStr<true>(initial_string, token_match);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(result);
#endif
  }
}

static void BM_StrFindWithSubStrFail(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithSubStr(initial_string, token_nomatch);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrFindWithSubStrFailLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithSubStr<true>(initial_string, token_nomatch);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

// The string being searched for is longer. Should be very fast as only length
// comparisons are needed.
static void BM_StrFindWithSubStrLong(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithSubStr(initial_string, token_nomatchlonger);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrFindWithSubStrLongLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithSubStr<true>(initial_string, token_nomatchlonger);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrFindWithCompare(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithCompare(initial_string, token_match);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(result);
#endif
  }
}

static void BM_StrFindWithCompareLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithCompare<true>(initial_string, token_match);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(result);
#endif
  }
}

static void BM_StrFindWithCompareFail(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithCompare(initial_string, token_nomatch);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrFindWithCompareFailLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithCompare<true>(initial_string, token_nomatch);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrFindWithCompareLong(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithCompare(initial_string, token_nomatchlonger);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrFindWithCompareLongLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithCompare<true>(initial_string, token_nomatchlonger);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrFindWithFirstOf(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithFirstOf(initial_string, token_match);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(result);
#endif
  }
}

static void BM_StrFindWithFirstOfLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithFirstOf<true>(initial_string, token_match);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(result);
#endif
  }
}

static void BM_StrFindWithFirstOfFail(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithFirstOf(initial_string, token_nomatch);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrFindWithFirstOfFailLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithFirstOf<true>(initial_string, token_nomatch);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrFindWithFirstOfLong(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithFirstOf(initial_string, token_nomatchlonger);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}
static void BM_StrFindWithFirstOfLongLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrFindWithFirstOf<true>(initial_string, token_nomatchlonger);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

// Register the function as a benchmark
BENCHMARK(BM_StrFindWithSubStr);
BENCHMARK(BM_StrFindWithSubStrLenCheck);
BENCHMARK(BM_StrFindWithSubStrFail);
BENCHMARK(BM_StrFindWithSubStrFailLenCheck);
BENCHMARK(BM_StrFindWithSubStrLong);
BENCHMARK(BM_StrFindWithSubStrLongLenCheck);
BENCHMARK(BM_StrFindWithCompare);
BENCHMARK(BM_StrFindWithCompareLenCheck);
BENCHMARK(BM_StrFindWithCompareFail);
BENCHMARK(BM_StrFindWithCompareFailLenCheck);
BENCHMARK(BM_StrFindWithCompareLong);
BENCHMARK(BM_StrFindWithCompareLongLenCheck);
BENCHMARK(BM_StrFindWithFirstOf);
BENCHMARK(BM_StrFindWithFirstOfLenCheck);
BENCHMARK(BM_StrFindWithFirstOfFail);
BENCHMARK(BM_StrFindWithFirstOfFailLenCheck);
BENCHMARK(BM_StrFindWithFirstOfLong);
BENCHMARK(BM_StrFindWithFirstOfLongLenCheck);

// NOLINTEND

// --------------------------------------------------------------------------
// std::string_view checks
// --------------------------------------------------------------------------

template <bool l = false>
static __attribute__((noinline)) auto StrVFindWithSubStr(
    const std::string_view& str, const std::string_view& substr) -> bool {
  auto token_len = substr.length();
  if (l)
    if (str.length() < token_len) return false;
  return substr == str.substr(0, token_len);
}

template <bool l = false>
static __attribute__((noinline)) auto StrVFindWithCompare(
    const std::string_view& str, const std::string_view& substr) -> bool {
  auto token_len = substr.length();
  if (l)
    if (str.length() < token_len) return false;
  return str.compare(0, token_len, substr) == 0;
}

template <bool l = false>
static __attribute__((noinline)) auto StrVFindWithFirstOf(
    const std::string_view& str, const std::string_view& substr) -> bool {
  if (l) {
    auto token_len = substr.length();
    if (str.length() < token_len) return false;
  }
  return str.find_first_of(substr, 0) == 0;
}

// NOLINTBEGIN

static std::string_view initial_string_v{"lighttpd_server.246"};
static std::string_view token_match_v{"lighttpd_server."};
static std::string_view token_nomatch_v{"systemd."};
static std::string_view token_nomatchlonger_v{
    "this_is_a_longer_string_that_doesnt_match"};

static void BM_StrVFindWithSubStr(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithSubStr(initial_string_v, token_match_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(result);
#endif
  }
}

static void BM_StrVFindWithSubStrLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithSubStr<true>(initial_string_v, token_match_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(result);
#endif
  }
}

static void BM_StrVFindWithSubStrFail(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithSubStr(initial_string_v, token_nomatch_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrVFindWithSubStrFailLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithSubStr<true>(initial_string_v, token_nomatch_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

// The string being searched for is longer. Should be very fast as only length
// comparisons are needed.
static void BM_StrVFindWithSubStrLong(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithSubStr(initial_string_v, token_nomatchlonger_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrVFindWithSubStrLongLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result =
        StrVFindWithSubStr<true>(initial_string_v, token_nomatchlonger_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrVFindWithCompare(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithCompare(initial_string_v, token_match_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(result);
#endif
  }
}

static void BM_StrVFindWithCompareLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithCompare<true>(initial_string_v, token_match_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(result);
#endif
  }
}

static void BM_StrVFindWithCompareFail(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithCompare(initial_string_v, token_nomatch_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrVFindWithCompareFailLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithCompare<true>(initial_string_v, token_nomatch_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrVFindWithCompareLong(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithCompare(initial_string_v, token_nomatchlonger_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrVFindWithCompareLongLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result =
        StrVFindWithCompare<true>(initial_string_v, token_nomatchlonger_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrVFindWithFirstOf(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithFirstOf(initial_string_v, token_match_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(result);
#endif
  }
}

static void BM_StrVFindWithFirstOfLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithFirstOf<true>(initial_string_v, token_match_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(result);
#endif
  }
}

static void BM_StrVFindWithFirstOfFail(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithFirstOf(initial_string_v, token_nomatch_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrVFindWithFirstOfFailLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithFirstOf<true>(initial_string_v, token_nomatch_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrVFindWithFirstOfLong(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrVFindWithFirstOf(initial_string_v, token_nomatchlonger_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrVFindWithFirstOfLongLenCheck(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result =
        StrVFindWithFirstOf<true>(initial_string_v, token_nomatchlonger_v);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

// Register the function as a benchmark
BENCHMARK(BM_StrVFindWithSubStr);
BENCHMARK(BM_StrVFindWithSubStrLenCheck);
BENCHMARK(BM_StrVFindWithSubStrFail);
BENCHMARK(BM_StrVFindWithSubStrFailLenCheck);
BENCHMARK(BM_StrVFindWithSubStrLong);
BENCHMARK(BM_StrVFindWithSubStrLongLenCheck);
BENCHMARK(BM_StrVFindWithCompare);
BENCHMARK(BM_StrVFindWithCompareLenCheck);
BENCHMARK(BM_StrVFindWithCompareFail);
BENCHMARK(BM_StrVFindWithCompareFailLenCheck);
BENCHMARK(BM_StrVFindWithCompareLong);
BENCHMARK(BM_StrVFindWithCompareLongLenCheck);
BENCHMARK(BM_StrVFindWithFirstOf);
BENCHMARK(BM_StrVFindWithFirstOfLenCheck);
BENCHMARK(BM_StrVFindWithFirstOfFail);
BENCHMARK(BM_StrVFindWithFirstOfFailLenCheck);
BENCHMARK(BM_StrVFindWithFirstOfLong);
BENCHMARK(BM_StrVFindWithFirstOfLongLenCheck);

// NOLINTEND

// --------------------------------------------------------------------------
// libc API
// --------------------------------------------------------------------------

static __attribute__((noinline)) auto StrCFindWithCompareN(
    const char* str, const char* substr) -> bool {
  auto token_len = strlen(substr);
  return strncmp(str, substr, token_len) == 0;
}

static __attribute__((noinline)) auto StrCFindWithCompare(
    const char* str, const char* substr) -> bool {
  return strcmp(str, substr) == 0;
}

// NOLINTBEGIN

static const char* initial_string_c = "lighttpd_server.246";
static const char* token_match_c = "lighttpd_server.";
static const char* token_nomatch_c = "systemd.";
static const char* token_nomatchlonger_c =
    "this_is_a_longer_string_that_doesnt_match";

static void BM_StrCFindWithCompareN(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrCFindWithCompareN(initial_string_c, token_match_c);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(result);
#endif
  }
}

static void BM_StrCFindWithCompareNFail(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrCFindWithCompareN(initial_string_c, token_nomatch_c);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrCFindWithCompareNLong(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrCFindWithCompareN(initial_string_c, token_nomatchlonger_c);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrCFindWithCompare(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrCFindWithCompare(initial_string_c, token_match_c);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(result);
#endif
  }
}

static void BM_StrCFindWithCompareFail(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrCFindWithCompare(initial_string_c, token_nomatch_c);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

static void BM_StrCFindWithCompareLong(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    auto result = StrCFindWithCompare(initial_string_c, token_nomatchlonger_c);
    benchmark::DoNotOptimize(result);
#ifdef NDEBUG
    assert(!result);
#endif
  }
}

// Register the function as a benchmark
BENCHMARK(BM_StrCFindWithCompareN);
BENCHMARK(BM_StrCFindWithCompareNFail);
BENCHMARK(BM_StrCFindWithCompareNLong);
BENCHMARK(BM_StrCFindWithCompare);
BENCHMARK(BM_StrCFindWithCompareFail);
BENCHMARK(BM_StrCFindWithCompareLong);

// Run the benchmark
BENCHMARK_MAIN();

// NOLINTEND
