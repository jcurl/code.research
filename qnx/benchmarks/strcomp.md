# String Comparison Benchmark <!-- omit in toc -->

C++ offers many different ways to compare strings. This benchmark provides some
mechanisms to use for comparison.

- [1. APIs Tested](#1-apis-tested)
- [2. Results](#2-results)
  - [2.1. Careful for Compiler Optimisations](#21-careful-for-compiler-optimisations)

## 1. APIs Tested

The following C++17 APIs are tested, which checks if a string starts with
another string. Note, C++20 is the first version with `startswith` method, but
is not tested as these benchmarks are for C++17.

| Benchmark              | API                               |
| ---------------------- | --------------------------------- |
| `StrFindWithSubStr*`   | `std::string::substr`             |
| `StrFindWithCompare*`  | `std::string::compare`            |
| `StrFindWithFirstOf*`  | `std::string::find_first_of`      |
| `StrVFindWithSubStr*`  | `std::string_view::substr`        |
| `StrVFindWithCompare*` | `std::string_view::compare`       |
| `StrVFindWithFirstOf*` | `std::string_view::find_first_of` |

Three variants are tested:

- The success case, where `str` starts with `substr`.
- The failing case, where `str` is shorter than `substr` and is not found (the
  first character is different).
- The failing case, where `str` is longer than `substr`, so should never match.

Additionally, the `libc` APIs are also tested:

| Benchmark               | API                     |
| ----------------------- | ----------------------- |
| `StrCFindWithCompareN*` | `strlen()`, `strncmp()` |
| `StrCFindWithCompare*`  | `strcmp() `             |

In each API tested:

- If the string starts with the sub-string.
- If the string doesn't start with the sub-string.
- If the string is shorter than the sub-string.
- If the string length should be compared first, before the string comparison.

## 2. Results

The CPU time is measured, as it is expected that the operations run
single-threaded and don't need any context switches.

| Benchmark                          | Ubuntu 22.04, i9-13950hx | RPi4, PiOS 5.3 | RPi4 QNX 7.1.0 | RPi4 QNX 8.0.0 | RPi5, PiOS 5.3 |
| ---------------------------------- | ------------------------ | -------------- | -------------- | -------------- | -------------- |
| BM_StrFindWithSubStr               | 30.4 ns                  | 95.9 ns        | 58.7 ns        | 68.0 ns        | 43.7 ns        |
| BM_StrFindWithSubStrLenCheck       | 33.3 ns                  | 91.5 ns        | 67.3 ns        | 70.7 ns        | 42.8 ns        |
| BM_StrFindWithSubStrFail           | 20.9 ns                  | 30.6 ns        | 32.0 ns        | 36.0 ns        | 14.5 ns        |
| BM_StrFindWithSubStrFailLenCheck   | 20.6 ns                  | 29.8 ns        | 33.3 ns        | 38.0 ns        | 14.0 ns        |
| BM_StrFindWithSubStrLong           | 28.0 ns                  | 79.2 ns        | 34.0 ns        | 36.0 ns        | 33.3 ns        |
| BM_StrFindWithSubStrLongLenCheck   | 2.11 ns                  | 6.01 ns        | 10.0 ns        | 10.7 ns        | 2.78 ns        |
| BM_StrFindWithCompare              | 7.53 ns                  | 22.0 ns        | 37.3 ns        | 35.3 ns        | 8.13 ns        |
| BM_StrFindWithCompareLenCheck      | 9.10 ns                  | 23.3 ns        | 39.3 ns        | 37.4 ns        | 8.75 ns        |
| BM_StrFindWithCompareFail          | 5.48 ns                  | 23.4 ns        | 22.7 ns        | 21.0 ns        | 8.75 ns        |
| BM_StrFindWithCompareFailLenCheck  | 7.34 ns                  | 24.7 ns        | 24.7 ns        | 22.8 ns        | 8.96 ns        |
| BM_StrFindWithCompareLong          | 5.48 ns                  | 22.4 ns        | 22.7 ns        | 21.2 ns        | 8.13 ns        |
| BM_StrFindWithCompareLongLenCheck  | 0.914 ns                 | 4.00 ns        | 6.67 ns        | 7.34 ns        | 1.67 ns        |
| BM_StrFindWithFirstOf              | 5.25 ns                  | 23.5 ns        | 10.0 ns        | 10.8 ns        | 9.38 ns        |
| BM_StrFindWithFirstOfLenCheck      | 5.81 ns                  | 24.6 ns        | 10.7 ns        | 12.3 ns        | 8.96 ns        |
| BM_StrFindWithFirstOfFail          | 21.5 ns                  | 62.1 ns        | 99.3 ns        | 101 ns         | 25.0 ns        |
| BM_StrFindWithFirstOfFailLenCheck  | 22.3 ns                  | 63.4 ns        | 100 ns         | 125 ns         | 24.8 ns        |
| BM_StrFindWithFirstOfLong          | 5.26 ns                  | 23.4 ns        | 27.3 ns        | 28.0 ns        | 8.96 ns        |
| BM_StrFindWithFirstOfLongLenCheck  | 0.914 ns                 | 4.00 ns        | 6.67 ns        | 8.00 ns        | 1.67 ns        |
| BM_StrVFindWithSubStr              | 3.43 ns                  | 12.7 ns        | 41.3 ns        | 27.3 ns        | 5.63 ns        |
| BM_StrVFindWithSubStrLenCheck      | 3.20 ns                  | 12.8 ns        | 40.0 ns        | 28.0 ns        | 6.25 ns        |
| BM_StrVFindWithSubStrFail          | 3.20 ns                  | 14.0 ns        | 6.67 ns        | 14.3 ns        | 6.46 ns        |
| BM_StrVFindWithSubStrFailLenCheck  | 3.65 ns                  | 14.7 ns        | 6.67 ns        | 14.2 ns        | 6.46 ns        |
| BM_StrVFindWithSubStrLong          | 0.917 ns                 | 4.00 ns        | 5.33 ns        | 4.67 ns        | 1.67 ns        |
| BM_StrVFindWithSubStrLongLenCheck  | 0.914 ns                 | 4.00 ns        | 5.33 ns        | 4.00 ns        | 1.67 ns        |
| BM_StrVFindWithCompare             | 3.66 ns                  | 16.0 ns        | 42.0 ns        | 26.7 ns        | 6.28 ns        |
| BM_StrVFindWithCompareLenCheck     | 3.20 ns                  | 12.7 ns        | 40.0 ns        | 27.3 ns        | 5.21 ns        |
| BM_StrVFindWithCompareFail         | 3.88 ns                  | 15.3 ns        | 8.00 ns        | 15.7 ns        | 6.46 ns        |
| BM_StrVFindWithCompareFailLenCheck | 3.65 ns                  | 14.0 ns        | 6.67 ns        | 14.6 ns        | 7.09 ns        |
| BM_StrVFindWithCompareLong         | 3.88 ns                  | 14.0 ns        | 8.00 ns        | 15.7 ns        | 6.51 ns        |
| BM_StrVFindWithCompareLongLenCheck | 0.914 ns                 | 4.00 ns        | 5.33 ns        | 4.00 ns        | 1.67 ns        |
| BM_StrVFindWithFirstOf             | 4.12 ns                  | 18.4 ns        | 7.33 ns        | 10.9 ns        | 6.88 ns        |
| BM_StrVFindWithFirstOfLenCheck     | 4.57 ns                  | 19.7 ns        | 8.00 ns        | 11.1 ns        | 6.88 ns        |
| BM_StrVFindWithFirstOfFail         | 18.4 ns                  | 64.1 ns        | 99.3 ns        | 123 ns         | 30.4 ns        |
| BM_StrVFindWithFirstOfFailLenCheck | 17.1 ns                  | 62.7 ns        | 100 ns         | 101 ns         | 29.8 ns        |
| BM_StrVFindWithFirstOfLong         | 4.11 ns                  | 22.0 ns        | 26.0 ns        | 25.3 ns        | 8.54 ns        |
| BM_StrVFindWithFirstOfLongLenCheck | 1.81 ns                  | 6.01 ns        | 5.33 ns        | 4.67 ns        | 2.57 ns        |
| BM_StrCFindWithCompareN            | 5.48 ns                  | 30.9 ns        | 52.0 ns        | 59.1 ns        | 12.9 ns        |
| BM_StrCFindWithCompareNFail        | 5.94 ns                  | 22.7 ns        | 20.0 ns        | 22.6 ns        | 10.0 ns        |
| BM_StrCFindWithCompareNLong        | 6.40 ns                  | 58.7 ns        | 24.0 ns        | 26.8 ns        | 14.2 ns        |
| BM_StrCFindWithCompare             | 2.74 ns                  | 20.0 ns        | 20.0 ns        | 20.0 ns        | 7.92 ns        |
| BM_StrCFindWithCompareFail         | 2.74 ns                  | 13.4 ns        | 12.7 ns        | 12.7 ns        | 5.84 ns        |
| BM_StrCFindWithCompareLong         | 2.74 ns                  | 13.4 ns        | 12.7 ns        | 12.7 ns        | 5.84 ns        |

Under GCC 11.4.0 with Ubuntu it was observed that C++ performance is generally
better (or not worse) when having a length check prior to the substring check,
should `substr` be longer than `str`.

The C implementation using `strnlen` needs a length first. The function can be
twice as fast (by not executing `strlen()` first) if the length is known
(`strncmp` and `strcmp` will have the same performance).

### 2.1. Careful for Compiler Optimisations

Initially, QNX 7.1.0 results seemed exceptionally fast (compared to QNX 8.0.0
and Linux), with almost all the `BM_StrVFindWith*` functions executing within
0.667ns. This lead to the conclusion that the compiler was completely optimising
the tests out of the loop. The solution to this is to ensure that the functions
doing the tests are not optimised with:

```cpp
static __attribute__((noinline)) auto StrVFindWithSubStr(const std::string_view& str, const std::string_view& substr) -> bool
static __attribute__((noinline)) auto StrVFindWithCompare(const std::string_view& str, const std::string_view& substr) -> bool
static __attribute__((noinline)) auto StrVFindWithFirstOf(const std::string_view& str, const std::string_view& substr) -> bool
```

instead of (with full optimisation in the compiler):

```cpp
static __attribute__((always_inline)) inline auto StrVFindWithSubStr(const std::string_view& str, const std::string_view& substr) -> bool
static __attribute__((always_inline)) inline auto StrVFindWithCompare(const std::string_view& str, const std::string_view& substr) -> bool
static __attribute__((always_inline)) inline auto StrVFindWithFirstOf(const std::string_view& str, const std::string_view& substr) -> bool
```

The static globals should *not* be defined as `const` in C++, as they can also
be further optimised, skewing the results. It is important to recognise that the
inputs are determined at runtime which can't be optimised, where the compiler in
this trivial test would not know that.
