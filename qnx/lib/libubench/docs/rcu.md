# RCU <!-- omit in toc -->

This is a research version of a user-space RCU class (read-copy-update).

- [1. Design](#1-design)
  - [1.1. Initial Investigations](#11-initial-investigations)
    - [1.1.1. Preallocated Memory and Atomic Behaviour](#111-preallocated-memory-and-atomic-behaviour)
  - [1.2. Algorithm](#12-algorithm)
    - [1.2.1. The RCU Pointer](#121-the-rcu-pointer)
    - [1.2.2. The RCU "Root"](#122-the-rcu-root)
      - [1.2.2.1. Analysis of Concurrent Behaviour](#1221-analysis-of-concurrent-behaviour)
      - [1.2.2.2. Freeing Memory](#1222-freeing-memory)
  - [1.3. References](#13-references)
    - [1.3.1. White Paper and `shared_ptr<T>`](#131-white-paper-and-shared_ptrt)
    - [1.3.2. RCU in the Linux Kernel](#132-rcu-in-the-linux-kernel)
    - [1.3.3. User Space Lock-Free RCU](#133-user-space-lock-free-rcu)
- [2. Performance Tests](#2-performance-tests)
  - [2.1. Compilation](#21-compilation)
    - [2.1.1. Linux Compilation](#211-linux-compilation)
    - [2.1.2. QNX Compilation and Target Preparation](#212-qnx-compilation-and-target-preparation)
  - [2.2. Execution](#22-execution)
  - [2.3. Results](#23-results)
    - [2.3.1. Test `rcutest`](#231-test-rcutest)
      - [2.3.1.1. Target: Raspberry Pi4B A72 QNX 7.1.0 (GCC 8.3.0)](#2311-target-raspberry-pi4b-a72-qnx-710-gcc-830)
      - [2.3.1.2. Target: Raspberry Pi4B A72 QNX 8.0.0 (GCC 12.2.0)](#2312-target-raspberry-pi4b-a72-qnx-800-gcc-1220)
      - [2.3.1.3. Target: Raspberry Pi4B A72 Linux 6.6.20+rpt-rpi-v8 (GCC 12.2.0)](#2313-target-raspberry-pi4b-a72-linux-6620rpt-rpi-v8-gcc-1220)
      - [2.3.1.4. Target: Raspberry Pi5 A76 Linux 6.6.31+rpt-rpi-2712 (GCC 12.2.0)](#2314-target-raspberry-pi5-a76-linux-6631rpt-rpi-2712-gcc-1220)
      - [2.3.1.5. Target: i7-6700T 2.80GHz Linux 6.5.0-45-generic (GCC 11.4.0)](#2315-target-i7-6700t-280ghz-linux-650-45-generic-gcc-1140)
      - [2.3.1.6. Target: Intel(R) Xeon(R) Silver 4410Y Linux 6.5.0-26-generic (GCC 11.4.0)](#2316-target-intelr-xeonr-silver-4410y-linux-650-26-generic-gcc-1140)
- [3. Future Work](#3-future-work)
  - [3.1. Build System](#31-build-system)
  - [3.2. Implementation](#32-implementation)

## 1. Design

The reads must be very fast and also work for multiple threads. It should use
lock-less programming when available to ensure that on contention the reader is
not blocked (which could be scheduled and stopped).

The writes are not thread-safe and must be serialised. They can be very slow and
context switches of threads are allowed. It is assumed that writes happen very
rarely.

When data is read, it is a snapshot at the time it is read. When there is an
update, the old data remains in memory, and is only freed when all readers
release the handle to the reference counting. Thus the amount of memory in use
in this implementation is unbounded.

The code is targetted for C++17.

As this uses atomic swapping of pointers, this library uses direct pointer
manipulation underneath, and the `new` and `delete` operator for allocating
memory.

### 1.1. Initial Investigations

An RCU effectively has three operations to define a read critical region and
then to update the data in the background while readers might still be in the
critical section.

`read_lock()`:

- On access, increment a reference counter
- Return pointer to the data

`read_unlock()`:

- Assume that a `read_lock()` has occurred prior
- Decrement the reference counter
- If the reference counter is zero, memory can be released

`write_copy_update()`:

- Perform a `read_lock()`
- Make a copy
- Assign to data atomically with a compare swap to the "Current" read pointer
  for the next `read_lock()` call.
- `read_unlock()` on the original set.

On closer observation, a `read_lock()` should be a construction of an object,
which makes the `read_unlock()` simpler as this can be implemented as a
destructor, i.e. the RAII pattern.

The work done by `read_lock()` should be as small as possible as this is the
most common operation expected.

When managing the references, we consider preallocated memory owned by a single
class, `rcu`. This memory is static and has the lifetime of the `rcu` class. The
`rcu` class gives out references via a `rcu_ptr` class which uses the references
from the `rcu` class. That implies the object from the `rcu` class must outlive
the lifetime of all other `rcu_ptr` objects that are handed out (else they'll
access invalid memory).

#### 1.1.1. Preallocated Memory and Atomic Behaviour

Using preallocated memory avoids memory allocation during the `read_lock()`
operation which reduces overhead further. Otherwise, a more complicated
technique is required such as "split reference counting" as a pointer is assumed
to consume a complete 64-bit value (the guaranteed size for an atomic swap for
64-bit architectures).

C++26 introduces "hazard pointers" and a `shared_ptr<T>` could be swapped
atomically simplifying the implementation significantly. However, this is not
available to us as we're targetting C++17.

Even though C++20 introduces atomic shared pointers
`std::atomic<std::shared_ptr<T>>`, most implementations are not lock-free
[CppCon 2023 Lock-free Atomic Shared Pointers Without a Split Reference Count?
-- Daniel
Anderson](https://isocpp.org/blog/2023/09/cppcon-2023-lock-free-atomic-shared-pointers-without-a-split-reference-coun)

### 1.2. Algorithm

Implementing an RCU is similar to a very simplified garbage collector in that:

- allocations are generally very fast.
- deallocations happen in another thread that is not time critical.

The RCU does not implement compaction. Deallocation would be done by a "writer"
thread during the `write_copy_update`, or could also be managed by a periodic
thread to clean up memory (although likely not needed for many use cases where
the ownership of a read object is usually short).

#### 1.2.1. The RCU Pointer

Define a `rcu_ptr` that has:

- A reference to a counter that is owned by another object `rcu` which must have
  a lifetime always longer than `rcu_ptr`.
- A pointer to a `const` object that never changes for the lifetime of the
  `rcu_ptr`.

It offers the following methods:

- A constructor with a reference to the counter and the pointer
- A copy constructor and assignment that increments the reference to the counter
- A move constructor and assignment that replaces the contents
- `use_count` to get the number of references. Usually, for an active pointer,
  it has a value of 2 or more (the first reference is the `rcu` root).
- `const T *get() const`
- `const T *operator->() const`
- `const T &operator*()`
- `explicit operator bool() const`

The destructor may be called, which will remove the reference. Note, if the
destructor is explicitly called, then it is called again when going out of scope
due to the implementation of an explicit destructor (that the destructor is not
"simple" due to dereferencing behaviour required).

#### 1.2.2. The RCU "Root"

The RCU root is given a `std::unique_ptr<T>` at construction, which it then
converts to a raw pointer and manages explicitly. Note, in C++17 this must be a
raw pointer, C++26 is the first standard to contain lock-free swapping of a
`std::unique_ptr` and `std::shared_ptr`.

Data structures:

- Assume an array, of size N, of the tuple (`ptr`, `refcount`). The size of each
  element is small, 16-bytes for a 64-bit machine. Call the array
  `std::array<ref, N> slot_`.
- There is an index `index_`, which is atomically updated, that points to the
  current active element in the array `slot_`.

The `read` Operation:

1. `i = index.load()`
2. `r = slot[i].refs.load()`
3. `if r == 0` go to 1.
4. `r' = r`
5. `r++`
6. `if !slot[i].exchange(r', r)`; go to 1.
7. `ptr = slots[i].ptr`
8. `return rcu_ptr(&slots[i].refs, ptr)`

The `update` Operation:

1. `i = (index.load() + 1) mod N`
2. `r = slot[i].ref.load()`
3. `if r != 0` then `i = (i + 1) mod N`. go to 2 unless at the original
   `index.load()`.
4. `slot[i].ref.store(1)`
5. `slot[i].ptr = ptr`
6. `old = swap(index, i)`
7. `r' = slot[old].ref.sub(1)`
8. `if r' == 1` then
   1. `dat = swap(slot[old].ptr, nullptr)`
   2. `if dat != nullptr` then `delete dat`

The `rcu_ptr` destructor Operation:

1. `r' = refs.sub(1)`. It is assumed the destructor is only called once.
   - In case this could be called twice (and certainly possible) set a flag that
     it has been called. We assume at the least the destructor cannot be called
     concurrently. That protects the case the user calls the destructor
     manually, and then as it exits scope or is deleted (often by unawares), we
     avoid undefined behaviour.
2. `if r' == 1` then
   1. free memory, the same way the `update` operation does.

The `rcu_ptr` destructor could free memory in a similar way to the `update`
frees memory.

##### 1.2.2.1. Analysis of Concurrent Behaviour

No race conditions are believed to exist, with the following assumptions:

- The `rcu` always outlives all `rcu_ptr` objects. If not, then the `rcu_ptr`
  objects will reference a counter in memory that is no longer existing.
- It is never possible to assign a `nullptr` via `update()`. Code enforces these
  checks.

##### 1.2.2.2. Freeing Memory

The current implementation frees memory in either the `rcu.update()` or the
`~rcu_ptr()` destructor. There might be a small impact on calling the destructor
for the pointer if it goes to the operating system to free memory.

An alternative would be to only have the `update` method free memory, by
iterating through all slots, and where the reference count is zero, then free
memory and mark the memory as being freed. This could make memory behaviour more
deterministic (a writer thread would control when memory is freed).

The write thread should be protected by a mutex that enforces serialisation of
the data updates.

### 1.3. References

#### 1.3.1. White Paper and `shared_ptr<T>`

There is a description [High-level C++ Implementation of the Read-Copy-Update
Pattern](https://martong.github.io/high-level-cpp-rcu_informatics_2017.pdf)
which looks interesting. It uses `shared_ptr<T>`. However the solution provided
here *does not work* as expected, as the atomic read of the `shared_ptr<T>` in
the lines below are *not* lockless:

```c++
std::shared_ptr<const T> read() const {
  return std::atomic_load_explicit(
    &sp, std::memory_order_consume);
}
```

This was confirmed on Ubuntu 22.04 with GCC 11.4 by single stepping assembly
(eventually `pthread_mutex_lock()` is called), and other C++17 API indicate it
is not lockless:

```c++
std::cout << std::atomic_is_lock_free(&sp) << std::endl;
```

The `copy_update` function may be doing more work, if the atomic
compare/exchange fails, then a second deep copy is made, which makes the update
routine more expensive than it should be.

The `shared_ptr<T>` is only atomic for all the `const` functions. The move/copy
constructors and destructor is not atomic. We don't yet have the ability to use
a C++20 `std::atomic<std::shared_ptr<T>>` which could be lockless (but not
tested).

We could try and roll our own shared pointer implementation, which reduces the
amount of code build and may be more performant. When doing so, the link [Shared
Pointers, An Introduction to Atomics in
C++](https://thecandcppclub.com/deepeshmenon/chapter-10-shared-pointers-and-atomics-in-c-an-introduction/781/).
The code presented is buggy (significant memory leaks), but the usage of the
`Deleter` can be employed in our own implementation that takes a
`std::unique_ptr<T>` and releases it for a raw pointer in the implementation
details.

#### 1.3.2. RCU in the Linux Kernel

The Linux Kernel implements the RCU in kernel space, but this can't be used in
user-space, as the final solution may not depend on the Linux kernel. The Linux
Kernel implements the free of the memory on context switches.

- [RCU Usage in the Linux
  Kernel](http://www2.rdrop.com/~paulmck/techreports/RCUUsage.2013.02.24a.pdf)
  - General information only
- [What is RCU, Fundamentally](https://lwn.net/Articles/262464/)
  - Generally describes using linked-lists for managing memory (like a garbage
    collection routine). Can be analysed further on what support we can achieve
    in a user-space implementation.
- [Performance of memory reclamation for lockless
  synchronization](https://sysweb.cs.toronto.edu/publication_files/0000/0159/jpdc07.pdf)
  - May be useful in types of algorithms that are performant, and how to test.

#### 1.3.3. User Space Lock-Free RCU

The GitHub repository [Simple and fast user-space RCU
library](https://github.com/ppetr/lockfree-userspace-rcu) is a C++
implementation that could be researched on it's usefulness.

It has not been checked for portability and has a few dependencies to
[Abseil-cpp](https://github.com/abseil/abseil-cpp/tree/807763a7f57dcf0ba4af7c3b218013e8f525e811)
which makes it less trivial. It also appears to have a worker thread in the
background to free memory periodically.

This design seems to be common, that someone else must free the memory. In this
implementation, by having a thread running in the background. In the Linux
Kernel by doing a free on context switch, which is equivalent (as described in
articles in the previous section) as having a thread in the kernel free memory.

One of the goals is to explore if this can be done differently.

## 2. Performance Tests

This section documents the performance test `RcuStress.DISABLED_ReadOps`. The
test runs for approximately 30 seconds.

For each hardware thread in the system, it creates an Operating System thread
that is in a tight loop getting an `rcu_ptr` and incrementing a counter, then
decrementing the reference by dropping the `rcu_ptr`.

In an additional thread, it will update the value of the pointer to new data, by
waiting 10ms on every wake. This means that the number of updates is less than
3000 (because a sleep) only guarantees a minimum sleep time.

### 2.1. Compilation

To compile for linux, compile release mode:

#### 2.1.1. Linux Compilation

```sh
mkdir -p build/linux && cd build/linux
cmake -S ../.. -B . -DCMAKE_BUILD_TYPE=Release
make
```

#### 2.1.2. QNX Compilation and Target Preparation

To compile for QNX, compile release mode:

```sh
. ./qnxsdp-env.sh
mkdir -p build/qnx7 && cd build/qnx7
cmake -S ../.. -B . -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../../toolchain/qcc_aarch64le.cmake
make
```

Under QNX, you'll also need to copy the libraries if they're not on your target
already:

- libregex.so.1
- libc++.so.1
- libcatalog.so.1

And you will need of course need to configure the `LD_LIBRARY_PATH`

```sh
export LD_LIBRARY_PATH=/dev/shmem
```

### 2.2. Execution

To run the test:

```sh
./rcutest -p1
```

### 2.3. Results

#### 2.3.1. Test `rcutest`

##### 2.3.1.1. Target: Raspberry Pi4B A72 QNX 7.1.0 (GCC 8.3.0)

| Threads | Updates | Reads     | Reads/sec/thread |
| ------- | ------- | --------- | ---------------- |
| 1       | 2727    | 383553524 | 12785117         |
| 2       | 2728    | 174759127 | 2912652          |
| 3       | 2728    | 165420314 | 1838003          |
| 4       | 2500    | 160741187 | 1339509          |

##### 2.3.1.2. Target: Raspberry Pi4B A72 QNX 8.0.0 (GCC 12.2.0)

| Threads | Updates | Reads     | Reads/sec/thread |
| ------- | ------- | --------- | ---------------- |
| 1       | 2728    | 383881579 | 12796052         |
| 2       | 2728    | 174247852 | 2904130          |
| 3       | 2728    | 160078564 | 1778650          |
| 4       | 2664    | 155756803 | 1297973          |

##### 2.3.1.3. Target: Raspberry Pi4B A72 Linux 6.6.20+rpt-rpi-v8 (GCC 12.2.0)

| Threads | Updates | Reads     | Reads/sec/thread |
| ------- | ------- | --------- | ---------------- |
| 1       | 2938    | 372720925 | 12424030         |
| 2       | 2983    | 164341832 | 2739030          |
| 3       | 2983    | 167824639 | 1864718          |
| 4       | 2981    | 155176172 | 1293134          |

##### 2.3.1.4. Target: Raspberry Pi5 A76 Linux 6.6.31+rpt-rpi-2712 (GCC 12.2.0)

| Threads | Updates | Reads      | Reads/sec/thread |
| ------- | ------- | ---------- | ---------------- |
| 1       | 2985    | 1293102770 | 43103425         |
| 2       | 2985    | 299227381  | 4987123          |
| 3       | 2984    | 239748124  | 2663868          |
| 4       | 2984    | 183437343  | 1528644          |

##### 2.3.1.5. Target: i7-6700T 2.80GHz Linux 6.5.0-45-generic (GCC 11.4.0)

| Threads | Updates | Reads      | Reads/sec/thread |
| ------- | ------- | ---------- | ---------------- |
| 1       | 2974    | 1522655515 | 50755183         |
| 2       | 2974    | 292123635  | 4868727          |
| 3       | 2980    | 251591951  | 2795466          |
| 4       | 2983    | 245511283  | 2045927          |
| 5       | 2983    | 247817975  | 1652119          |
| 6       | 2978    | 229300718  | 1273892          |
| 7       | 2984    | 232632030  | 1107771          |
| 8       | 2984    | 240477582  | 1001989          |

##### 2.3.1.6. Target: Intel(R) Xeon(R) Silver 4410Y Linux 6.5.0-26-generic (GCC 11.4.0)

| Threads | Updates | Reads      | Reads/sec/thread |
| ------- | ------- | ---------- | ---------------- |
| 1       | 2983    | 1667101517 | 55570050         |
| 2       | 2983    | 245107040  | 4085117          |
| 3       | 2983    | 114285794  | 1269842          |
| 4       | 2983    | 115300577  | 960838           |
| 5       | 2982    | 119191492  | 794609           |
| 6       | 2983    | 124185789  | 689921           |
| 7       | 2983    | 127739171  | 608281           |
| 8       | 2983    | 125469105  | 522787           |
| 9       | 2983    | 126174897  | 467314           |
| 10      | 2982    | 135241517  | 450805           |
| 11      | 2982    | 134774947  | 408408           |
| 12      | 2982    | 141818884  | 393941           |
| 13      | 2982    | 152494869  | 391012           |
| 14      | 2982    | 152175336  | 362322           |
| 15      | 2982    | 151951092  | 337669           |
| 16      | 2982    | 161342559  | 336130           |
| 17      | 2982    | 157299437  | 308430           |
| 18      | 2982    | 163461291  | 302706           |
| 19      | 2982    | 164255236  | 288167           |
| 20      | 2981    | 175015860  | 291693           |
| 21      | 2980    | 174246468  | 276581           |
| 22      | 2980    | 167670720  | 254046           |
| 23      | 2980    | 177260910  | 256899           |
| 24      | 2979    | 168028876  | 233373           |
| 25      | 2979    | 170364070  | 227152           |
| 26      | 2979    | 174466958  | 223675           |
| 27      | 2979    | 178006386  | 219760           |
| 28      | 2979    | 176460917  | 210072           |
| 29      | 2979    | 171361691  | 196967           |
| 30      | 2979    | 184372382  | 204858           |
| 31      | 2979    | 176122201  | 189378           |
| 32      | 2979    | 180545339  | 188068           |
| 33      | 2976    | 167540454  | 169232           |
| 34      | 2979    | 174513680  | 171091           |
| 35      | 2979    | 180811643  | 172201           |
| 36      | 2978    | 166970621  | 154602           |
| 37      | 2977    | 174375612  | 157095           |
| 38      | 2977    | 172444778  | 151267           |
| 39      | 2976    | 167614797  | 143260           |
| 40      | 2977    | 170048892  | 141707           |
| 41      | 2978    | 168851417  | 137277           |
| 42      | 2977    | 172821673  | 137160           |
| 43      | 2979    | 175795027  | 136275           |
| 44      | 2973    | 169588668  | 128476           |
| 45      | 2980    | 171586474  | 127101           |
| 46      | 2980    | 165350377  | 119819           |
| 47      | 2981    | 169002080  | 119859           |
| 48      | 2980    | 163818048  | 113762           |

## 3. Future Work

### 3.1. Build System

- [ ] Update [Sanitizers](https://github.com/arsenm/sanitizers-cmake).

### 3.2. Implementation

Further implementation details should be done:

- [ ] Should use the Deleter provided by the `std::unique_ptr<T>`.
- [ ] The atomic exchanges should be optimised to use the correct memory
  ordering. New measurements.
