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
cmake -S ../.. -B . -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../../toochain/qcc_aarch64le
make -j8
```

## Memory Allocation `malloc_bench`

Measures the time on the "default" usage of the `malloc` c-library call.

See [malloc.md](./benchmarks/malloc.md)
