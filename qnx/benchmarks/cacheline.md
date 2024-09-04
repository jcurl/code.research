# Cache-line Benchmark

Goal: Measure empirically the size of the cache by measuring the time it takes
to do strided copies.

Based on [Daniel Lemire - Measuring the Size of the Cache Line
Empirically](https://lemire.me/blog/2023/12/12/measuring-the-size-of-the-cache-line-empirically/).

This implementation uses Google Benchmark to capture the results. It measures
the time it takes to copy 256MB of data from one array to another array (the
larger sizes are to ignore the impacts an L3 cache, which can be up to 36MB in
size on more modern processors).

## Interpreting the Results

To measure the cache size, look at the time it takes to do the copy, which is
output from Google Benchmark in units of nanoseconds. See
[results](./cacheline/results/README.md) for information how to run and capture
the data.

The formula for plotting the data is: `bytes / (time / slice)` where:

- bytes is 256 * 1024 * 1024 in the code (configurable with the `-b` option)
  - As there are two loops, the total amount of data copied is the buffer size.
    e.g. a slice of 16 is 16 runs of copying 1/16th of the data.
- time is measured in nanoseconds
- slice is the byte increment for each new byte copy, starting at 16 and
  incrementing by one until 512.

The graph is expected to be flat for the slice up until the cache-line, as each
byte read is the cache-line in size, anything smaller is not faster.

While Daniel Lemire uses "GB/s" for the y-axis, I find this confusing as it is
not the transfer speed. We're not calculating the memory bus bandwidth at all in
this test (we don't know how much memory is being read per byte copy, only the
overall access time).

The loop being measured is:

```c
  for (unsigned int s = 0; s < slice; s++) {
    for (unsigned int i = 0; i < size; i += slice) {
      arr2[i] = arr1[i];
    }
  }
```

As the `slice` is increased, the total number of copy operations with `arr2[i] =
arr1[i]` remains approximately constant (as the outer loop decreases).

For the
case that the `slice` is within the cache-line, the inner loop speed remains
constant (and thus the total time of the measurement increases approximately
linearly with the slice as L1 cache access is significantly faster than memory
access).

This is what causes the flat line at the start of the graph (we're copying the
entire cache-line for one byte access), so the value `time/slice` remains
constant because the `time` is the total time for `slice * inner_loop` and the
time for the `inner_loop` is constant. So it is showing effectively the `size /
(time_one_cache-line_copy + time_overhead)`. The `inner_loop` is expected to be
the same time for slices from n..cache-line, because the first access within the
cache-line takes the most time, the remaining requests are of order of a few
nano-seconds for the L1 cache access.

So double the slice from 16 to 32, the time also doubles for the same number of
copy operations, and the resultant graph remains flat.

Double from the cache-line size (e.g. 64 to 128) and the inner loop should now
decrease in time, because less cache-lines are being accessed than before, the
outer loop causes the overall time now to remain constant. Hence the graph shows
an increasing value proportional to the slice, because each access is now
reading a new cache-line from memory which is much slower than the L1 cache
