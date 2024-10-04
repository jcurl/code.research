# Introduction

This document covers some basics when updating and adding code to this
repository.

## Development Environment

The following systems are used for development:

- Ubuntu 20.04, 22.04
- CMake 3.14
- GCC 8.3.0 or later with C++17

Tools required for Developing

- Clang
- Clang-Tidy
- Clang-Format

## Checking in Code

Ensure that code you create is formatted with `clangformat` before submitting.

```sh
make clangformat
```

Try and test on all possible targets:

| OS / Architecture | x86 | x86_64 | RPi 4 | RPi 5 |
| ----------------- | --- | ------ | ----- | ----- |
| Ubuntu 20.04      | -   | X      | -     | -     |
| Ubuntu 22.04      | -   | X      | -     | -     |
| RPi OS            | -   | -      | X     | X     |
| QNX 7.1.0         | -   | -      | X     | -     |
| QNX 8.0.0         | -   | -      | X     | -     |
