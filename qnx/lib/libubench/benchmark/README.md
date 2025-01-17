# UBench Library Benchmarks

This directory is for creating benchmarks that define the implementation inside
the `ubench` library contained in the `src` directory.

In particular, string functions are often used and are known bottlenecks when
implementing code. Even if this appears to be used only _once_, and one argues
it's not worth the effort to benchmark and optimise, code gets copied and pasted
everywhere. At least give an attempt to one of the biggest performance impacts
for people looking for a quick solution.

This directory is _not_ intended for general benchmarking.
