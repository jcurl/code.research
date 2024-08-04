# RCU <!-- omit in toc -->

This is a research version of a user-space RCU class (read-copy-update).

- [1. Compilation](#1-compilation)
  - [1.1. Using CMake](#11-using-cmake)
    - [1.1.1. Compiler Options](#111-compiler-options)
    - [Code Coverage](#code-coverage)
  - [1.2. Debugging with Visual Studio Code](#12-debugging-with-visual-studio-code)
  - [1.3. Clang-Tidy](#13-clang-tidy)
  - [1.4. Formatting](#14-formatting)
- [2. Design](#2-design)
  - [2.1. Initial Investigations](#21-initial-investigations)
    - [2.1.1. Preallocated Memory and Atomic Behaviour](#211-preallocated-memory-and-atomic-behaviour)
  - [2.2. Algorithm](#22-algorithm)
    - [2.2.1. The RCU Pointer](#221-the-rcu-pointer)
    - [2.2.2. The RCU "Root"](#222-the-rcu-root)
      - [2.2.2.1. Analysis of Concurrent Behaviour](#2221-analysis-of-concurrent-behaviour)
      - [2.2.2.2. Freeing Memory](#2222-freeing-memory)
  - [2.3. References](#23-references)
    - [2.3.1. White Paper and `shared_ptr<T>`](#231-white-paper-and-shared_ptrt)
    - [2.3.2. RCU in the Linux Kernel](#232-rcu-in-the-linux-kernel)
    - [2.3.3. User Space Lock-Free RCU](#233-user-space-lock-free-rcu)
- [3. Future Work](#3-future-work)
  - [3.1. Build System](#31-build-system)
  - [3.2. Implementation](#32-implementation)

## 1. Compilation

Instructions provided work for Ubuntu 22.04.

### 1.1. Using CMake

Build (default with Debug Mode) with:

```sh
mkdir build && cd build
cmake ..
make
```

#### 1.1.1. Compiler Options

Use the following options when building with CMake:

- Build in debug mode.
  ```sh
  cmake ..
  ```

  Adds `-DDEBUG`. Enabled clang-tidy.

- Build in release mode.

  ```sh
  cmake .. -DCMAKE_BUILD_TYPE=Release

  Still compiles unit tests and runs clang-tidy.

- Disable Clang-Tidy (enabled by default)

  ```sh
  cmake .. -DENABLE_CLANG_TIDY=off
  ```

- Disable Google Test (enabled by default)

  ```sh
  cmake .. -DENABLE_TEST=off
  ```
#### Code Coverage

Enable code covergae:

```sh
cmake .. -DENABLE_TEST=on -DCODE_COVERAGE=on
```

To execute test cases and see coverage (the command is `ccov-<testname>`).

```sh
make ccov-rcutest-test
```

Then the output will tell you to open your browser with the correct file.

### 1.2. Debugging with Visual Studio Code

Use the Microsoft C/C++ extension (v1.20.5) which provides a language server and
debugging capabilities. The `.vscode/launch.json` contains the command to debug
this RCU binary.

`Ctrl-Shift-B` should build the *debug* binaries.

### 1.3. Clang-Tidy

On Ubuntu 22.04, you need to have installed `clang-tidy` for the checks. It
would fail on loading the header files, while GCC would compile quite happily.

I had to make sure that
[`libstdc++-12-dev](https://askubuntu.com/questions/1443701/clang-cant-find-headers-but-gcc-can)
was installed. I also installed `libc++abi-dev` but that wasn't enough.

```sh
sudo apt install libc++abi-dev
sudo apt install ibstdc++-12-dev
```

Then I could compile.

### 1.4. Formatting

```sh
make clangformat
```

Ensure that the `CMakeLists.txt` has been updated to also add the files to
format in the list (not just the target).

## 2. Design

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

### 2.1. Initial Investigations

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

#### 2.1.1. Preallocated Memory and Atomic Behaviour

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

### 2.2. Algorithm

Implementing an RCU is similar to a very simplified garbage collector in that:

- allocations are generally very fast.
- deallocations happen in another thread that is not time critical.

The RCU does not implement compaction. Deallocation would be done by a "writer"
thread during the `write_copy_update`, or could also be managed by a periodic
thread to clean up memory (although likely not needed for many use cases where
the ownership of a read object is usually short).

#### 2.2.1. The RCU Pointer

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

#### 2.2.2. The RCU "Root"

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

##### 2.2.2.1. Analysis of Concurrent Behaviour

No race conditions are believed to exist, with the following assumptions:

- The `rcu` always outlives all `rcu_ptr` objects. If not, then the `rcu_ptr`
  objects will reference a counter in memory that is no longer existing.
- It is never possible to assign a `nullptr` via `update()`. Code enforces these
  checks.

##### 2.2.2.2. Freeing Memory

The current implementation frees memory in either the `rcu.update()` or the
`~rcu_ptr()` destructor. There might be a small impact on calling the destructor
for the pointer if it goes to the operating system to free memory.

An alternative would be to only have the `update` method free memory, by
iterating through all slots, and where the reference count is zero, then free
memory and mark the memory as being freed. This could make memory behaviour more
deterministic (a writer thread would control when memory is freed).

The write thread should be protected by a mutex that enforces serialisation of
the data updates.

### 2.3. References

#### 2.3.1. White Paper and `shared_ptr<T>`

There is a description [High-level C++ Implementation of the Read-Copy-Update
Pattern](https://martong.github.io/high-level-cpp-rcu_informatics_2017.pdf)
which looks interesting. It uses `shared_ptr<T>`. However the solution provided
here _does not work_ as expected, as the atomic read of the `shared_ptr<T>` in
the lines below are _not_ lockless:

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

#### 2.3.2. RCU in the Linux Kernel

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

#### 2.3.3. User Space Lock-Free RCU

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

## 3. Future Work

### 3.1. Build System

- [ ] Update [Sanitizers](https://github.com/arsenm/sanitizers-cmake).
- [ ] Add QNX 7.1.0 for testing.

### 3.2. Implementation

Further implementation details should be done:

- [ ] Consider a test program `main.cpp` that sets up n threads, n-1 threads
  read as fast as possible, the nth which writes occasionally. Then we can use
  this for testing the cache behaviour and performance.
- [ ] Should use the Deleter provided by the `std::unique_ptr<T>`.
- [ ] The atomic exchanges should be optimised to use the correct memory
  ordering.