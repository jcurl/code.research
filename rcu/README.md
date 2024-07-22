# RCU <!-- omit in toc -->

This is a research version of a user-space RCU class (read-copy-update).

- [1. Compilation](#1-compilation)
  - [1.1. Using CMake](#11-using-cmake)
  - [1.2. Debugging with Visual Studio Code](#12-debugging-with-visual-studio-code)
  - [1.3. Clang-Tidy](#13-clang-tidy)
  - [1.4. Formatting](#14-formatting)
- [2. Design](#2-design)
  - [2.1. Initial Investigations](#21-initial-investigations)
    - [2.1.1. White Paper and `shared_ptr<T>`](#211-white-paper-and-shared_ptrt)
    - [2.1.2. RCU in the Linux Kernel](#212-rcu-in-the-linux-kernel)
    - [2.1.3. User Space Lock-Free RCU](#213-user-space-lock-free-rcu)
  - [2.2. Analysis of Race Conditions](#22-analysis-of-race-conditions)
- [3. Future Work](#3-future-work)
  - [3.1. Build System](#31-build-system)
  - [3.2. Implementation](#32-implementation)

## 1. Compilation

Instructions provided work for Ubuntu 22.04.

### 1.1. Using CMake

Build with:

```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=DEBUG
make
```

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
destructor, i.e. the RAII pattern. There is a similar pattern in the STL
implemented by `shared_ptr<T>` where `T` should be `const`.

The work done by `read_lock()` should be as small as possible as this is the
most common operation expected.

#### 2.1.1. White Paper and `shared_ptr<T>`

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

#### 2.1.2. RCU in the Linux Kernel

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

#### 2.1.3. User Space Lock-Free RCU

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

### 2.2. Analysis of Race Conditions

There are numerous control points that must be analysed between a thread called
`rcu.read()` and another thread calling `rcu.update()` in parallel.

The most important one is the `rcu.read()` in parallel with the `rcu.write()`.
In the analysis, assume that no instance has a read copy, which is the most
likely scenario when memory could be freed resulting in a race condition (if
there is a read copy, then the reference count will never reach zero and so
memory will never be freed, resulting in no race condition, until there's an
update).

We assume that the `rcu.update()` is not multithread safe API and thus the user
must serialize access through a mutex or other similar lock guard.

| Read thread                    | Write thread                   | rcu.node_ | Notes                       |
| ------------------------------ | ------------------------------ | --------- | --------------------------- |
| `rcu.read()`                   |                                |           |                             |
| `auto node = node_.load()`     |                                | 1         | Node 1: (ref = 1, ptr = a)  |
|                                | `rcu.update(p)`                | 1         |                             |
|                                | `*rptr = new rcu_ptr<T>(..)`   | 1         |                             |
|                                | `*old = node_.exchange(rptr);` | 2         |                             |
|                                | `delete old`                   | 2         | Deletes node 1              |
|                                |                                | 2         | Node 1: (ref = 0, ptr = a)  |
|                                | `delete ptr_`                  | 2         | Node 1: delete a            |
|                                |                                | 2         | Node 1: delete refs         |
| `new_node = rcu_ptr<T>(*node)` |                                |           | CRASH (UB). Inc invalid ptr |

With the table above, we see that we cannot easily avoid the pointer to the
shared reference counter, and the memory owned by it. At any time, we could
potentially reference deallocated memory resulting in undefined behaviour. The
dereference to the pointer and the update must be atomic.

Further investigation should be done that the "rcu" class owns the reference,
not the rcu_ptr. Then the `ref` is our guard. This would necessitate
preallocating the memory for the pointer/references upfront to a fixed size.

To workaround this, we'd need to implement a lock on the RCU datastructure, that
the reference to object containing the reference counter and pointer to the data
is atomic. As this is greater than 64-bits this is difficult.

Some implementations try and pack reference counts and data into a 48-bit
pointer and 8-bit counter, but this is not suitable for newer systems, where
Level 5 paging uses 57-bit virtual address space, and immediately we can observe
incorrect assumptions (or significant impediments) to this approach.

## 3. Future Work

### 3.1. Build System

- [ ] Add Google Test.
- [ ] Update [Sanitizers](https://github.com/arsenm/sanitizers-cmake).
- [ ] Add QNX 7.1.0 for testing.

### 3.2. Implementation

- [ ] Consider a test program `main.cpp` that sets up 4 threads, three the read
  as fast as possible, the fourth which writes occasionally. Then we can use
  this for testing the cache behaviour and performance.
- [ ] Should use the Deleter provided by the `std::unique_ptr<T>`.
- [ ] The atomic exchanges should be optimised to use the correct memory
  ordering.
- [ ] We could avoid allocating `rcu_ptr` on the heap with `rcu::update` and use
  an array instead. Then the method should return a `bool` if it succeeded or
  not.