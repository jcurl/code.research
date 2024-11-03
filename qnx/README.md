# Operating System Microbenchmarks <!-- omit in toc -->

- [1. Compilation](#1-compilation)
- [2. Installed Binaries](#2-installed-binaries)
  - [2.1. Benchmarks](#21-benchmarks)
    - [2.1.1. Memory Allocation](#211-memory-allocation)
    - [2.1.2. Cache Line](#212-cache-line)
    - [2.1.3. String Comparison](#213-string-comparison)
  - [2.2. Tools](#22-tools)
    - [2.2.1. Time Comparison](#221-time-comparison)
    - [2.2.2. Core Latency](#222-core-latency)
    - [2.2.3. Network Performance with UDP Load](#223-network-performance-with-udp-load)

## 1. Compilation

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

See further instructions for the tools for specific build instructions (such as
enabling assembly instruction sets, etc.).

## 2. Installed Binaries

### 2.1. Benchmarks

#### 2.1.1. Memory Allocation

Measures the time on the "default" usage of the `malloc` c-library call.

- Tool: `malloc_bench`
- Supported: Linux, QNX 7.1, QNX 8.0
- [Documentation](./benchmarks/malloc.md)
- [Results](./benchmarks/malloc/results/README.md)

Other Platforms:

- (X) MacOS-X: Generates invalid results.

#### 2.1.2. Cache Line

Estimate the cache line size by measuring the time it takes to do strided
copies.

- Tool: `cacheline_bench`
- Supported: Linux, QNX 7.1, QNX 8.0
- [Documentation](./benchmarks/cacheline.md)
- [Results](./benchmarks/cacheline/results/README.md)

#### 2.1.3. String Comparison

Determine the fastest way to check if a string starts with another string in
C++.

- Tool: `strcmp_bench`
- Supported: Linux, QNX 7.1, QNX 8.0
- [Documentation](./benchmarks/strcomp.md)
- [Results](./benchmarks/strcomp.md)

### 2.2. Tools

#### 2.2.1. Time Comparison

Shows the C++ resolution, if they're steady clocks. Prints out the following
clocks, and they can be used to estimate the sources. For example, the clock
`std::chrono::high_resolution_clock` has a different behaviour on QNX 7.1, than
Linux (QNX is a monotonic clock based on `ClockCycles()`, where on Linux it's
based on `CLOCK_REALTIME`).

- Tool: `time_compare`
- Supported: Linux, QNX 7.1, QNX 8.0
- [Documentation](./tools/time_compare.md)
- [Results](./tools/time_compare.md)

#### 2.2.2. Core Latency

Measure the cache-to-cache latency between cores.

- Tool: `core_latency`
- Supported: Linux, QNX 7.1, QNX 8.0
- [Documentation](./tools/core_latency.md)
- [Results](./tools/core_latency.md)

To enable LSE for ARM (e.g. Raspberry Pi 5), you must add the appropriate
compiler options, e.g.

```sh
cmake -B . -S .. -DCMAKE_CXX_FLAGS="armv8.1-a+lse"
```

Other Platforms:

- (X) MacOS-X: Pinning threads in the Operating System not supported.

#### 2.2.3. Network Performance with UDP Load

Generate a constant rate of UDP traffic and measure system load.

- Tool: `udp_load`
- Supported: Linux, QNX 7.1, QNX 8.0
- [Documentation](./tools/udp_load.md)
- [Results](./tools/udp_load/results/results.md)
