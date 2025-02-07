# UBench Library Benchmarks <!-- omit in toc -->

This directory is for creating benchmarks that define the implementation inside
the `ubench` library contained in the `src` directory.

In particular, string functions are often used and are known bottlenecks when
implementing code. Even if this appears to be used only _once_, and one argues
it's not worth the effort to benchmark and optimise, code gets copied and pasted
everywhere. At least give an attempt to one of the biggest performance impacts
for people looking for a quick solution.

This directory is _not_ intended for general benchmarking.

- [1. Strings](#1-strings)

## 1. Strings

The default routines for parsing hex strings are generally slower than what are
possible. The `ubench::string::from_chars_hex` provides an alternative that is
about 2-3x faster by optimising for base 16, and not supporting `0x` at the
front. Additionally, it does fewer checks, but users can still perform simple
errors if the entire string was parsed properly or not.

Run on various OSes

- QNX 7.1.0 1500MHz
- RPi OS (GCC 12.2.0) 1500MHz

| Benchmark              | QNX 7.1.0 |   Linux |
| ---------------------- | --------: | ------: |
| BM_StrToL_byte         |   74.1 ns | 36.0 ns |
| BM_StrToL_short        |   82.7 ns | 48.0 ns |
| BM_StrToL_long         |    114 ns | 68.7 ns |
| BM_StrToL_llong        |    198 ns |  115 ns |
| BM_StrToL_xlong        |    350 ns |  188 ns |
| BM_StrToLL_byte        |   70.0 ns | 36.0 ns |
| BM_StrToLL_short       |   84.6 ns | 48.0 ns |
| BM_StrToLL_long        |    105 ns | 68.7 ns |
| BM_StrToLL_llong       |    164 ns |  115 ns |
| BM_StrToLL_xlong       |    290 ns |  188 ns |
| BM_StrToUL_byte        |   73.4 ns | 34.7 ns |
| BM_StrToUL_short       |   80.7 ns | 47.7 ns |
| BM_StrToUL_long        |    111 ns | 66.1 ns |
| BM_StrToUL_llong       |    161 ns |  105 ns |
| BM_StrToUL_xlong       |    347 ns |  179 ns |
| BM_StrToULL_byte       |   73.0 ns | 37.7 ns |
| BM_StrToULL_short      |   84.8 ns | 47.7 ns |
| BM_StrToULL_long       |    102 ns | 66.1 ns |
| BM_StrToULL_llong      |    158 ns |  105 ns |
| BM_StrToULL_xlong      |    291 ns |  179 ns |
| BM_FromChars_byte      |    233 ns | 17.4 ns |
| BM_FromChars_short     |    283 ns | 25.4 ns |
| BM_FromChars_long      |    375 ns | 33.4 ns |
| BM_FromChars_llong     |    553 ns | 49.4 ns |
| BM_FromChars_xlong     |    836 ns | 87.1 ns |
| BM_FromCharsHex_ubyte  |    110 ns | 14.0 ns |
| BM_FromCharsHex_ushort |    143 ns | 18.0 ns |
| BM_FromCharsHex_ulong  |    204 ns | 24.7 ns |
| BM_FromCharsHex_ullong |    319 ns | 40.7 ns |
| BM_FromCharsHex_uxlong |    608 ns | 80.1 ns |
| BM_FromCharsHex_byte   |    109 ns | 14.0 ns |
| BM_FromCharsHex_short  |    145 ns | 18.0 ns |
| BM_FromCharsHex_long   |    204 ns | 24.7 ns |
| BM_FromCharsHex_llong  |    318 ns | 40.7 ns |
| BM_FromCharsHex_xlong  |    589 ns | 80.1 ns |
