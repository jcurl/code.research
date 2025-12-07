# Introduction <!-- omit in toc -->

The subproject `cpuid-x86` is a simple tool to query the CPUID information on
x86 computers. It consists of a cross platform library to allow multiple ways to
query the CPUID, from the CPUID instruction, to device drivers that do the
query.

It has little practical use except to dump information in various ways. One
observation is that testing on a tool such as `valgrind` returns emulated
results for the CPUID instruction, where using a kernel provided device driver
provides the correct results.

- [1. Component Design](#1-component-design)
- [2. Library Detailed Design](#2-library-detailed-design)
  - [2.1. Implementation Specific CPUID](#21-implementation-specific-cpuid)
  - [2.2. CPUID Context](#22-cpuid-context)
  - [2.3. Dumping](#23-dumping)

## 1. Component Design

The main work is done by the `libcpuid` library, which is specific for the x86
processor. Other components build on top of the library to get CPUID information.

![](./assets/cpuid/cpuid_component.svg)

## 2. Library Detailed Design

The `libcpuid` C++17 static library is designed to be built depending on the
features available from the Operating System. It provides multiple
implementations that a user can choose, depending how they wish to get the
information.

Reading the CPUID via the Intel instruction `cpuid` is done with the class
`cpuidreader_native`.

![](./assets/libcpuid/libcpuid_cpuid_native.svg)

Reading the CPUID via the Linux kernel module `cpuid.ko` is done with the class
`cpuidreader_dev`.

![](./assets/libcpuid/libcpuid_cpuid_dev.svg)

### 2.1. Implementation Specific CPUID

The classes `cpuidreader_*` are intended to be the classes that are instantiated
to read the CPUID information. They all offer the same methods, that makes them
suitable for template programming:

- `has_cpuid()` indicates if the class is capable of returning results.
- `is_online()` indicates if the results are for the current.
- `cores()` the number of cores available to query.
- `cpuid()` query the CPUID results for the leaf and subleaf.
- `enable_core()` provide a context, that when destroyed, releases the context,
  for querying the CPUID.

### 2.2. CPUID Context

The context is specific to each implementation of `cpuidreader_*`. The context
should be used to ensure the results for querying the CPUID instruction occurs
on a specific core. There should only be one context active at any one time
(nested contexts should not be allowed, but may not result in an error either).

Essentially, running the method `cpuid()` without a context, returns a default
result, depending on the implementaiton (e.g. always core 0, or the current
thread, whatever makes sense for that specific implementation).

When CPUID information is needed for a specific core, one generates a context
through the `enable_core()` method. That returns an object, and so long as that
object is alive, calls to `cpuid()` are valid for that context.

- For the `cpuidreader_native` implementation, it configures the current
  executing thread to be pinned on a specific core. While the context is active,
  then the thread can only run on a specific core. Once the context is released,
  the original run-mask is resumed.
- For the `cpuidreader_dev` implementation, the context opens a specific device
  path `/dev/cpu/N/cpuid` which is used to query the CPUID information. The
  default context is to open core 0.

Each context returns the following fields:

- `bool()` if the context is active (if the creation of the context succeeded
  and can be used).
- `error()` the system `errno` if the context creation failed, to be looked up
  by the Operating System manual.
- `core()` the core that the context is valid for.

Creating a new context while an existing context is open results in undefined
behaviour.

### 2.3. Dumping

The method `cpuid_dump` takes a `cpuidreader` and uses this to dump CPUID
information on the core specified. THe results are provided as a vector.

The implementation is reasonably simple, there is no generic interface. The
`cpuid_dump` queries the `cpuidreader` for the first leaf to get the brand
string. From this, it calls a specialised method internally to get all
information for that specific CPU.

Each CPU has methods for obtaining information, one per leaf. As CPU
implementations offer share the same specifications, dumping for an AMD might
call methods originally documented for dumping for an Intel.

With this, there are three files (no classes are created):

- `dump_generic.h` dump the regions 0x00000000 (normal), 0x80000000 (extended,
  defined by AMD), 0x40000000 (hypervisor extensions). Each block has the first
  register which defines the number of leaves, followed by the data. None of the
  dumping routines dump subleaves.
- `dump_intel.h` dump Intel specific registers. There are lots of overlap with
  AMD, so many methods here can be used by AMD and other CPUs also.
- `dump_amd.h` dump AMD specific registers.
