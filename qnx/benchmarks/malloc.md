# Malloc Benchmark

Goal: Measure the time it takes to allocate memory and free memory on the
target. It measures the following:

- `malloc()` and `free()`
- `malloc()` only (with overhead due to pausing/resuming timing)
- `free()` only (with overhead due to pausing/resuming timing)

With these three measurements, we can estimate the time for a malloc/free and
take into account the overhead for time measurements in each loop.

Taking the measurements is not exact, as google-benchmark runs many tests in a
single loop that does a `malloc()` and a `free()`. Using the functions
`PauseTiming()` and `ResumeTiming()` introduce overhead that skew the results,
but we can estimate the relative times.

- BM_MallocFreeBench = T_m + T_f
- BM_MallocBench = T_m + T_p
- BM_FreeBench = T_f + T_p

So, we should be able to calculate the overhead for timing instructions by:

- T_p = (BM_MallocBench + BM_FreeBench - BM_MallocFreeBench) / 2

There will be error due to the clock timing, so the value of T_p is expected to
vary most for very fast loops. Execution is expected to be measured over
real-time on an idle system, do not use the CPU time as this won't include the
time the Operating System requires to manage memory (set up the MMU, etc.).

This doesn't account for the time a `malloc()` might need for the first call. A
more complicated test would be needed to measure the memory allocation for the
first time when a process starts. Also be careful of some Operating Systems
having a "zero-page pool", where they use idle time to clear memory making
potential `malloc()` calls faster by preparing zero-pages before the call.

## Results

Results are recorded using the command

```sh
./benchmarks/malloc/malloc_bench --benchmark_format=json > ../../benchmarks/malloc/results/xxx.json
```

| OS                                               | Processor                                | Results                                                                   |
| ------------------------------------------------ | ---------------------------------------- | ------------------------------------------------------------------------- |
| Ubuntu 22.04 6.5.0-45-generic #45~22.04.1-Ubuntu | Intel(R) Core(TM) i7-6700T CPU @ 2.80GHz | [Skylake Ubuntu 22.04](./malloc/results/skylake-i7-6700T_ubuntu2204.json) |
| QNX 7.1.0                                        | Raspberry Pi4B A72                       | [RPi4B QNX 7.1.0](./malloc/results/rpi4_qnx710.json)                      |
| QNX 8.0.0                                        | Raspberry Pi4B A72                       | [RPi4B QNX 8.0.0](./malloc/results/rpi4_qnx800.json)                      |
| RPi OS 5.3                                       | Raspberry Pi4B A72                       | [RPi4B RPiOS](./malloc/results/rpi4_linux.json)                           |
| RPi OS 5.3                                       | Raspberry Pi 5 A76                       | [RPi5 RPiOS](./malloc/results/rpi5_linux.json)                            |
