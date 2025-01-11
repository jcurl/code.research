# Introduction <!-- omit in toc -->

This document covers some basics when updating and adding code to this
repository.

- [1. Project Goals](#1-project-goals)
  - [1.1. Secondary Goals](#11-secondary-goals)
  - [1.2. Project Non-Goals](#12-project-non-goals)
- [2. Development Environment](#2-development-environment)
- [3. Target Platforms](#3-target-platforms)
  - [3.1. Tier 1](#31-tier-1)
  - [3.2. Tier 2](#32-tier-2)
- [4. Submitting Patches](#4-submitting-patches)
  - [4.1. Checking in Code](#41-checking-in-code)
    - [4.1.1. Target Language](#411-target-language)
    - [4.1.2. Clang-Tidy](#412-clang-tidy)
    - [4.1.3. Clang-Format](#413-clang-format)
    - [4.1.4. Tier 1 Targets](#414-tier-1-targets)
  - [4.2. Separation between Library Code and Tools](#42-separation-between-library-code-and-tools)
  - [4.3. CMake](#43-cmake)

## 1. Project Goals

The project aims to provide simple microbenchmarking tools that can be used to
compare Unix-like systems. This is a research project. It should be easy for
users to build on the Tier 1 targets using the GCC compiler.

### 1.1. Secondary Goals

Portability: Support for other targets may exist. Other targets bring value in
showing how different Unix systems do their work, enable testing on platforms
that might otherwise not be available. Microbenchmarks may depend on drivers and
deliver interesting comparisons.

As part of portability, decisions on code separation must be made, separation
using the build tool CMake, and can lead to better, maintainable software (think
about using the CMake build system to choose different files for compilation for
larger changes, or macros for smaller variants). It can serve as an example for
other esoteric platforms.

Support for other compilers. GCC isn't the only compiler, and it has bugs. As
part of the research we should not disregard correctness and design.

### 1.2. Project Non-Goals

Code written should still be pragmatic (a goal).

Don't overuse abstractions and generic libraries and reusability and shareable
code. Share code only when there is a use case for it. This occurs typically for
Operating System abstractions when supporting multiple platforms, to keep the
rest of the code maintainable by reducing variant handling. When writing a
function, check if another tool does something similar and refactor it into the
reusable library when it's known it will be used by two or more benchmarks.

Unit testing is not a goal. This is a small tools and microbenchmark repository,
it is not intended to be used in production. For that, people are expected to
copy the code using the license and do what they need. Code should be small and
simple enough that it's testable manually and easily. So no Operating System
abstraction layer and trying to emulate that. Test instead on the targets of
interest. If it's too hard to test manually, then it likely doesn't belong here.
Consider the goals above and figure out how to separate variants to reduce
complexity of testing.

## 2. Development Environment

The following systems are used for development:

- Ubuntu 22.04
- CMake 3.22.1
- podman version 3.4.4
  - Allows building in Ubuntu 20.04, 22.04, 24.04 within a container. See
    `scripts/build.sh`
- GCC 8.3.0 or later with C++17 (the standard compiler toolchain with the OS
  used)

Tools required for developing and maintaining code quality (building is
optional):

- Clang 14 or later.
- Clang-Tidy 14 or later.
- Clang-Format 14 or later.

## 3. Target Platforms

### 3.1. Tier 1

The following platforms are targetted for running benchmarks. Not all tools may
build, but the build system should disable such tools that are not supported:

- Linux (Ubuntu 20.04 and later)
  - CMake 3.16 and later
  - GCC 8.3.0 and later
- QNX 7.1
- QNX 8.0

Target Compiler is GCC for the platform.

CMake is used as the build system. CMake is the defacto C++ multi-platform build
system, despite some of its shortcomings. Most developers in C++ are already
familiar with CMake and most people already have all the necessary tooling to
build projects based on CMake. CMake is significantly simpler to write for, and
use, than automake.

See the `scripts/Makefile` for running the target `linux` for the various
targets, using `podman` under Linux (Ubuntu 22.04 and later). This automates the
checks before committing changes.

### 3.2. Tier 2

There may be patches for other platforms. The support for other platforms are
not continuously tested, may result in data that is incorrect. However, there is
value in such platforms for the sake of portability and designing code to handle
the different variants.

An example of occasional support for Tier 2 targets are:

- NetBSD 10.1 (RPi4 cross-compile)¹
- FreeBSD 14.2 (RPi4 cross-compile)²
- Cygwin 3.x (Windows 10 64-bit or later)
- MacOS-X (non-free, support very limited)

Notes:
- ¹ Build from `scripts` with `make netbsd`
- ² Build from `scripts` with `make freebsd`

## 4. Submitting Patches

### 4.1. Checking in Code

#### 4.1.1. Target Language

The language targetted is C++17, which is available on the Tier 1 targets. No
C++20 or later features as not all Tier 1 targets support C++20 sufficiently.

#### 4.1.2. Clang-Tidy

Clang-Tidy should be installed on your machine and findings should be fixed or
suppressed in comments.

#### 4.1.3. Clang-Format

Clangformat is integrated with the build system and runs formatting on the files
that are part of the project / target platform you're developing.

Ensure that code you create is formatted with `clangformat` before submitting.

```sh
make clangformat
```

#### 4.1.4. Tier 1 Targets

Try and test on all possible targets (see the `scripts` folder for automatic
Ubuntu targets):

| OS / Architecture | x86 | x86_64 | RPi 4 | RPi 5 |
| ----------------- | --- | ------ | ----- | ----- |
| Ubuntu 20.04      | -   | X      | -     | -     |
| Ubuntu 22.04      | -   | X      | -     | -     |
| Ubuntu 24.04      | -   | X      | -     | -     |
| RPi OS            | -   | -      | X     | X     |
| QNX 7.1           | -   | -      | X     | -     |
| QNX 8.0           | -   | -      | X     | -     |

### 4.2. Separation between Library Code and Tools

In general, code should be written and placed for the tool. Not everything needs
to be generalised or put under a library.

Guidelines when to put in the `libubench` library would be:

- Common functions that abstract the Operating System (such as timers, thread
  pinning, networking). It is especially useful for variant handling to be done
  in the library.
- If more than one tool requires the code. In this case, refactor the existing
  tool and put it in the library.

By minimising the code in the library, it becomes less burdensome to retest all
the tools on the supported targets. Existing test results remain generally
valid. Breaking targets through isolation of code is reduced.

By putting non-essential, reusable code, that is Operating Specific, code is
simpler to maintain without affecting test results.

### 4.3. CMake

Each tool builds a binary, but in itself is not a project. They must target
C++17.

Rules for checks should use functionality from CMake 3.14 (Modern CMake) which
is the lowest common denominator for the Tier 1 targets. Toolchain files are
provided for QNX 7.1 and QNX 8.

Create a Module file for more complex tasks. Remember that this is a C++
project, but be aware that CMake treats C as different to C++ and may need
testing for each language (e.g. see [ARM64
LSE](../cmake/modules/arm/arm64_has_lse.cmake)).

Instead of putting modules in the project folder, they are at the top level.
