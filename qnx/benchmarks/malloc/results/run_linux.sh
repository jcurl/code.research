#!/bin/bash

# Execute benchmarks for Linux. Ensure to capture the console output for later
# processing.

BASE_NAME=$1
echo ----------------------------------------------------------------------
echo ${BASE_NAME}.console.txt
taskset -c 1 ./benchmarks/malloc/malloc_bench --benchmark_out_format=json --benchmark_out=${BASE_NAME}.json

echo ----------------------------------------------------------------------
echo ${BASE_NAME}_threshold0.console.txt
taskset -c 1 ./benchmarks/malloc/malloc_bench -mM_TRIM_THRESHOLD=0 --benchmark_out_format=json --benchmark_out=${BASE_NAME}_threshold0.json

echo ----------------------------------------------------------------------
echo ${BASE_NAME}_pad0.console.txt
taskset -c 1 ./benchmarks/malloc/malloc_bench -mM_TRIM_THRESHOLD=0,M_TOP_PAD=0 --benchmark_out_format=json --benchmark_out=${BASE_NAME}_pad0.json

echo ----------------------------------------------------------------------
echo ${BASE_NAME}_mmap.console.txt
taskset -c 1 ./benchmarks/malloc/malloc_bench -mM_MMAP_THRESHOLD=0 --benchmark_out_format=json --benchmark_out=${BASE_NAME}_mmap.json

echo ----------------------------------------------------------------------
echo ${BASE_NAME}_mlockall.console.txt
taskset -c 1 ./benchmarks/malloc/malloc_bench -L --benchmark_out_format=json --benchmark_out=${BASE_NAME}_mlockall.json

echo ----------------------------------------------------------------------
echo ${BASE_NAME}_mmap.console.txt
taskset -c 1 ./benchmarks/malloc/malloc_bench -L -mM_MMAP_THRESHOLD=0 --benchmark_out_format=json --benchmark_out=${BASE_NAME}_mmap_mlockall.json
