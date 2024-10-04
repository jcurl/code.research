# Operating System Microbenchmarks

To compile for Linux:

```sh
mkdir -p build/linux
cd build/linux
cmake -S ../.. -B . -DCMAKE_BUILD_TYPE=Release
make -j8
```

To compile for QNX

```sh
. ./qnxsdp-env.sh
mkdir -p build/qnx7
cd build/qnx7
cmake -S ../.. -B . -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../../toolchain/qcc_aarch64le
make -j8
```

## Memory Allocation `malloc_bench`

Measures the time on the "default" usage of the `malloc` c-library call.

See [malloc.md](./benchmarks/malloc.md)

## Time Comparison

Shows the C++ resolution, if they're steady clocks. Prints out the following
clocks, and they can be used to estimate the sources. For example, the clock
`std::chrono::high_resolution_clock` has a different behaviour on QNX 7.1, than
Linux (QNX is a monotonic clock based on `ClockCycles()`, where on Linux it's
based on `CLOCK_REALTIME`).

See [time_compare.md](./tools/time_compare.md)

## Core Latency

Measure the cache-to-cache latency between cores.

See [core_latency.md](./tools/core_latency.md)

To enable LSE for ARM, you must add the appropriate compiler options, e.g.

```sh
cmake -B . -S .. -DCMAKE_CXX_FLAGS="armv8.1-a+lse"
```

## Cache Line

Estimate the cache line size by measuring the time it takes to do strided
copies.

See [cacheline.md](./benchmarks/cacheline.md) and
[results](./benchmarks/cacheline/results/README.md).
