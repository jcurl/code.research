# Operating System Microbenchmarks <!-- omit in toc -->

- [1. Compilation](#1-compilation)
  - [1.1. Tier 1 Architecture Specific](#11-tier-1-architecture-specific)
    - [1.1.1. Linux (RPi4, RPi5, Intel)](#111-linux-rpi4-rpi5-intel)
    - [1.1.2. QNX 7.1, 8.0 (Linux Host, RPi4 Target)](#112-qnx-71-80-linux-host-rpi4-target)
  - [1.2. Docker Hosted Toolchains](#12-docker-hosted-toolchains)
  - [1.3. Further Notes](#13-further-notes)
  - [1.4. Other Architectures (Not Supported)](#14-other-architectures-not-supported)
    - [1.4.1. Cygwin 3.x (Windows)](#141-cygwin-3x-windows)
- [2. Installed Binaries](#2-installed-binaries)
  - [2.1. Benchmarks](#21-benchmarks)
    - [2.1.1. Memory Allocation](#211-memory-allocation)
    - [2.1.2. Cache Line](#212-cache-line)
    - [2.1.3. String Comparison](#213-string-comparison)
  - [2.2. Tools](#22-tools)
    - [2.2.1. Time Comparison](#221-time-comparison)
    - [2.2.2. Core Latency](#222-core-latency)
    - [2.2.3. Network Performance with UDP Load](#223-network-performance-with-udp-load)
    - [2.2.4. List QNX Files](#224-list-qnx-files)
    - [2.2.5. List Shared Memory](#225-list-shared-memory)
    - [2.2.6. List Interfaces and IPv4 Addresses](#226-list-interfaces-and-ipv4-addresses)
- [3. Further Benchmarks for Testing](#3-further-benchmarks-for-testing)
  - [3.1. String Interning](#31-string-interning)
  - [3.2. Conversion of Hexadecimal String to Integer](#32-conversion-of-hexadecimal-string-to-integer)
  - [3.3. Printing of Network Strings](#33-printing-of-network-strings)
  - [3.4. Read/Copy/Update Userspace Implementation](#34-readcopyupdate-userspace-implementation)
- [4. Proof of Concept Code](#4-proof-of-concept-code)
  - [4.1. UDPv4 with Fragmentation](#41-udpv4-with-fragmentation)

## 1. Compilation

### 1.1. Tier 1 Architecture Specific

#### 1.1.1. Linux (RPi4, RPi5, Intel)

To compile for Linux:

```sh
mkdir -p build/linux
cd build/linux
cmake -S ../.. -B . -DCMAKE_BUILD_TYPE=Release
make -j8
```

#### 1.1.2. QNX 7.1, 8.0 (Linux Host, RPi4 Target)

To compile for QNX

```sh
. ./qnxsdp-env.sh
mkdir -p build/qnx7
cd build/qnx7
cmake -S ../.. -B . -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../../toolchain/qcc_aarch64le.cmake
make -j8
```

### 1.2. Docker Hosted Toolchains

See the `scripts` folder for using `podman` to build for Linux, NetBSD and
FreeBSD targets inside a container. See documentation
[README.md](./scripts/README.md) for further information. More development
information is in [Developer.md](./DEVELOPER.md).

### 1.3. Further Notes

See further instructions for the tools for specific build instructions (such as
enabling assembly instruction sets, etc.).

### 1.4. Other Architectures (Not Supported)

#### 1.4.1. Cygwin 3.x (Windows)

To compile on Cygwin 64-bit under Windows 10 or later:

```sh
mkdir -p build/cygwin
cd build/cygwin
cmake -S ../.. -B . -DCMAKE_BUILD_TYPE=Release
make -j8
```

## 2. Installed Binaries

### 2.1. Benchmarks

#### 2.1.1. Memory Allocation

Measures the time on the "default" usage of the `malloc` c-library call.

- Tool: `malloc_bench`
- Supported: Linux, QNX 7.1, QNX 8.0
- [Documentation](./benchmarks/malloc.md)
- [Results](./benchmarks/malloc/results/README.md)

Other Platforms:

- (!) Cygwin (Windows): No customisations for malloc parameters supported.
- (!) NetBSD 10.1: No customisations for malloc parameters supported.
- (!) FreeBSD 14.2: No customisations for malloc parameters supported.

#### 2.1.2. Cache Line

Estimate the cache line size by measuring the time it takes to do strided
copies.

- Tool: `cacheline_bench`
- Supported: Linux, QNX 7.1, QNX 8.0
- [Documentation](./benchmarks/cacheline.md)
- [Results](./benchmarks/cacheline/results/README.md)

Other Platforms:

- (!) Cygwin (Windows): Runs. Values not checked.
- (!) NetBSD 10.1: Runs. Values not checked.
- (!) FreeBSD 14.2: Runs. Values not checked.

#### 2.1.3. String Comparison

Determine the fastest way to check if a string starts with another string in
C++.

- Tool: `strcmp_bench`
- Supported: Linux, QNX 7.1, QNX 8.0
- [Documentation](./benchmarks/strcomp.md)
- [Results](./benchmarks/strcomp.md)

Other Platforms:

- (/) Cygwin (Windows): Runs.
- (/) NetBSD 10.1: Runs.
- (/) FreeBSD 14.2: Runs.

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

Other Platforms:

- (/) Cygwin (Windows): Runs.
- (/) NetBSD 10.1: Runs.
- (/) FreeBSD 14.2: Runs.

#### 2.2.2. Core Latency

Measure the cache-to-cache latency between cores.

- Tool: `core_latency`
- Supported: Linux, QNX 7.1, QNX 8.0
- [Documentation](./tools/core_latency.md)
- [Results](./tools/core_latency.md)

To enable LSE for ARM (e.g. Raspberry Pi 5), you must add the appropriate
compiler options, e.g.

```sh
cmake -B . -S .. -DCMAKE_CXX_FLAGS="-march=armv8.1-a+lse"
```

Other Platforms:

- (X) MacOS-X: Pinning threads in the Operating System not supported.
- (/) Cygwin (Windows): Shows core latencies as expected.
- (/) NetBSD 10.1: Shows core latencies as expected (tested on RPi4).
- (/) FreeBSD 14.2: Shows core latencies as expected (tested on RPi4).

#### 2.2.3. Network Performance with UDP Load

Generate a constant rate of UDP traffic and measure system load.

- Tool: `udp_load`
- Supported: Linux, QNX 7.1, QNX 8.0
- [Documentation](./tools/udp_load.md)
- [Results](./tools/udp_load/results/results.md)

Other Platforms:

- (X) Cygwin (Windows): Works. Values should be informational only.
- (!) NetBSD 10.1: Runs. IDLE/CPU time not accurate. BPF measurements fail on
  multi-thread tests.
- (/) FreeBSD 14.2: Results provided.

#### 2.2.4. List QNX Files

Show all files opened by a process under QNX, like in `pidin`.

- Tool: `lsqf`
- Supported: QNX 7.1, QNX 8.0
- [Documentation](./tools/lsqf.md)

#### 2.2.5. List Shared Memory

Check the mapping for all PIDs and look for shared memory pages.

- Tool: `lshmem`
- Supported: QNX 7.1, QNX 9.0
- [Documentation](./tools/lshmem.md)

#### 2.2.6. List Interfaces and IPv4 Addresses

Uses the library abstraction to list all interfaces and the properties.

- Tool `iplist`
- Supported: Linux, QNX 7.1, QNX 8.0
- [Documentation](./tools/iplist.md)

Other Platforms:

- (/) Cygwin (Windows): Gets the alias names from the GUIDs. No VLANs.
- (/) NetBSD 10.1: Works.
- (/) FreeBSD 14.2: Works.

## 3. Further Benchmarks for Testing

### 3.1. String Interning

Various implementations for string interning are implemented.

See [README.md](./lib/libubench/benchmark/str_intern/README.md)

### 3.2. Conversion of Hexadecimal String to Integer

A general implementation with lookups for a fast conversion of a hex string to a
number.

See [README.md](./lib/libubench/benchmark/README.md)

### 3.3. Printing of Network Strings

What is the more "generic" way to convert a MAC address and an IPv4 address to a
string.

See [netbench.cpp comments](./lib/libubench/benchmark/net_bench.cpp)

### 3.4. Read/Copy/Update Userspace Implementation

Shows performance of updates for an RCU. You'll see that it serializes to the
speed of one core (expected).

See [rcu.md](./lib/libubench/docs/rcu.md)

## 4. Proof of Concept Code

Proof of Concept code is a small binary to test a particular idea, or to check
functionality across operating systems to observe their behaviour.

### 4.1. UDPv4 with Fragmentation

Extended from the tool `udp_load`, constructs a UDP packet and sends it over a
BPF interface. It demonstrates how to create IPv4 checksums and do packet
fragmentation. Operating systems that don't have the BSD BPF interface do the
same thing with the network stack.

See [README.md](./poc/bpf_udp4/docs/README.md)
