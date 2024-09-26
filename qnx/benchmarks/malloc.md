# Malloc Benchmark <!-- omit in toc -->

Goal: Measure the time it takes to allocate memory and free memory on the
target. It measures the following:

- `malloc()` and `free()`
- `malloc()` only (with overhead due to pausing/resuming timing)
- `free()` only (with overhead due to pausing/resuming timing)

Contents:

- [1. Description of the Tests](#1-description-of-the-tests)
- [2. Results](#2-results)
- [3. Details of Tests](#3-details-of-tests)
  - [3.1. Memory Options](#31-memory-options)
  - [3.2. Memory Options Testing](#32-memory-options-testing)
    - [3.2.1. Massif profiling](#321-massif-profiling)
    - [3.2.2. Massif Backtraces](#322-massif-backtraces)
    - [3.2.3. GLibC options](#323-glibc-options)
    - [3.2.4. Monitoring Local Memory Usage](#324-monitoring-local-memory-usage)
  - [3.3. Memory Locking](#33-memory-locking)

## 1. Description of the Tests

The `BM_MallocFreeBench` is the simplest test, allocating a block of memory of a
particular size and freeing it. Usually, the first allocation is slow, when it
actually needs to go to the kernel to get the memory, and subsequent allocations
are fast after a free, as the memory actually remains mapped to the process, but
available from the heap for the next `malloc` call.

## 2. Results

See [Results](./malloc/results/README.md) for measured data and comparisons.

## 3. Details of Tests

For detailed information about the test cases, see the [Jupyter
Notebook](./malloc/results/analyse.ipynb).

### 3.1. Memory Options

The tool allows overriding some memory options with the command line
`-mOPTION=value`. This parses the `OPTION` and calls `mallopt` with `value`.
These can be used to observe changes in performance behaviour.

See the next section on testing. If allocation of memory by the system runtime
may have an effect, often environment variables can be used instead of this
command line option to change behaviour.

### 3.2. Memory Options Testing

The `malloc_bench` tool allows additional options, `-mXXX=n`, where the `XXX` is
an option to the API `mallopt`, and `n` is the value, as given by the Operating
System vendor. These values can be used to influence the behaviour of `malloc()`
and observe their resutls.

As such, the benchmark overrides the global `new` and `delete` operators, to
avoid the usage of dynamic heap memory. The only usage of heap memory that
should be allowed is by the benchmarks themselves.

#### 3.2.1. Massif profiling

To see [where heap memory](https://valgrind.org/docs/manual/ms-manual.html) is
being performed:

```sh
$ cmake -B . -S .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-g"
$ make
$ valgrind --tool=massif --threshold=0.0./benchmarks/malloc/malloc_bench --benchmark_filter=XX
==132397== Massif, a heap profiler
==132397== Copyright (C) 2003-2017, and GNU GPL'd, by Nicholas Nethercote
==132397== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
==132397== Command: ./benchmarks/malloc/malloc_bench --benchmark_filter=XX
==132397==
Failed to match any benchmarks against regex: XX
Used total local memory 0 of 8388608 bytes.
Used by benchmarks 0
==132397==
$ ms_print --threshold=0.0 massif.out.132397 > massif.txt
```

It's observed that during the initialisation phase, heap memory is allocated by
the runtime.

Rule: Do not allocate memory through static initialisers. This will not use the
overridden `new`/`delete` operators.

As such the benchmarks are initialised by the `main()` function, and not
globally.

#### 3.2.2. Massif Backtraces

One will observe in the output, despite overriding `new` and `new[]`, there are
still back traces, indicating there are allocations. This is not true (reviewing
the code from Google Benchmark, uses the standard library, which should use
`new` and not `malloc` internally).

e.g. take the following output:

```txt
--------------------------------------------------------------------------------
  n        time(i)         total(B)   useful-heap(B) extra-heap(B)    stacks(B)
--------------------------------------------------------------------------------
 32      3,040,522           80,952           79,448         1,504            0
 33      3,052,615           80,952           79,448         1,504            0
 34      3,064,896           80,952           79,448         1,504            0
 35      3,077,381           80,952           79,448         1,504            0
 36      3,089,866           80,952           79,448         1,504            0
98.14% (79,448B) (heap allocation functions) malloc/new/new[], --alloc-fns, etc.
->89.81% (72,704B) 0x490F939: ??? (in /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.30)
|     ->89.81% (72,704B) 0x4006567: _dl_init (dl-init.c:117)
|       ->89.81% (72,704B) 0x40202C9: ??? (in /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2)
|
->04.20% (3,400B) in 26 places, all below massif's threshold (1.00%)
|
->02.85% (2,304B) 0x12CEDA: allocate (new_allocator.h:127)
| ->02.85% (2,304B) 0x12CEDA: allocate (alloc_traits.h:464)
|   ->02.85% (2,304B) 0x12CEDA: _M_allocate (stl_vector.h:346)
|     ->02.85% (2,304B) 0x12CEDA: void std::vector<std::vector<long, std::allocator<long> >, std::allocator<std::vector<long, std::allocator<long> > > >::_M_realloc_insert<std::vector<long, std::allocator<long> > >(__gnu_cxx::__normal_iterator<std::vector<long, std::allocator<long> >*, std::vector<std::vector<long, std::allocator<long> >, std::allocator<std::vector<long, std::allocator<long> > > > >, std::vector<long, std::allocator<long> >&&) (vector.tcc:440)
|       ->02.85% (2,304B) 0x126C3E: emplace_back<std::vector<long int, std::allocator<long int> > > (vector.tcc:121)
|         ->02.85% (2,304B) 0x126C3E: push_back (stl_vector.h:1204)
|           ->02.85% (2,304B) 0x126C3E: benchmark::internal::Benchmark::Range(long, long) (benchmark_register.cc:255)
|             ->02.85% (2,304B) in 3 places, all below massif's threshold (1.00%)
|
->01.28% (1,040B) 0x4BDD791: __new_exitfn (cxa_atexit.c:114)
      ->01.28% (1,040B) 0x11345B: __static_initialization_and_destruction_0(int, int) [clone .constprop.0] (iostream:74)
          ->01.28% (1,040B) 0x4BC1EBA: __libc_start_main@@GLIBC_2.34 (libc-start.c:379)
```

We could think that `benchmark::internal::Benchmark::Range(long, long)
(benchmark_register.cc:255)` is allocating memory, which however it is not
allocating from the heap. This is because `massif` provides its own
implementation of `new`.

This can be confirmed in the output, by printing how much memory the overload in
`allocator.cpp` provides:

```txt
==132397==
Failed to match any benchmarks against regex: XX
Used total local memory 0 of 8388608 bytes.
Used by benchmarks 0
==132397==
```

Note the values are `0`.

#### 3.2.3. GLibC options

To show if memory is being used by the `main()` function, we can instrument by
printing the amount of memory allocated by `allocator.cpp`, as well as the API
[`malloc_stats()](https://man7.org/linux/man-pages/man3/malloc_stats.3.html).

By capturing the statistics at the very first function, then capturing
statistics again after the call:

```cpp
auto main(int argc, char** argv) -> int {
  std::size_t pre_alloc0 = get_allocated_localmem();
  malloc_stats();

  ...

  mallopt_options options(argc, argv);
  if (!options.do_run()) return options.result();

  malloc_stats();
  std::size_t pre_alloc1 = get_allocated_localmem();
  std::cout << "memory used " << pre_alloc0 << std::endl;
  std::cout << "memory used " << pre_alloc1 << std::endl;
```

we can see that all heap allocations are actually being done by the library
before `main` is entered. The `malloc_stats` shows no change, and there is an
increase of the `localmem` from `allocator.cpp` showing this is correctly being
used.

```txt
$ ./benchmarks/malloc/malloc_bench --benchmark_filter=XX
Arena 0:
system bytes     =     135168
in use bytes     =      74432
Total (incl. mmap):
system bytes     =     135168
in use bytes     =      74432
max mmap regions =          0
max mmap bytes   =          0

Arena 0:
system bytes     =     135168
in use bytes     =      74432
Total (incl. mmap):
system bytes     =     135168
in use bytes     =      74432
max mmap regions =          0
max mmap bytes   =          0
memory used 2096
memory used 29040
allocated block at 0x798e15e00010
Failed to match any benchmarks against regex: XX
Used total local memory 139456 of 8388608 bytes.
Used by benchmarks 110416
```

#### 3.2.4. Monitoring Local Memory Usage

Because we've overridden the `new` operators, the tool will print how much
memory was used at the end of the test (the memory is never freed as an
optimisation).

If memory could not be allocated, the program will `std::abort()`.

This can be used to identify if more memory should be given in the `allocator.h`
file.

### 3.3. Memory Locking

Some Operating Systems provide system wide memory locking, that memory which is
mapped is guaranteed to be backed by physical memory pages. Others don't have
this, but offer the system call `mlockall()`.

The `mlockall()` is a better metrics than trying to walk pages, as it can show a
faster allocation of memory (and usage) in some scenarios.
