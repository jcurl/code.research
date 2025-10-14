# The QNX devctl Benchmark <!-- omit from toc -->

The purpose of the benchmark is to test the throughput of message passing in
real-world scenarios, specifically using the `devctl` library call of QNX. This
results in a message being constructed and sent to a server. The server receives
this message, does some work, and replies.

## 1. Design

There are two components

```text
+-----------+                 +--------+
| dev-adder |-------o)--------| client |
| (server)  |   /dev/adder/n  |        |
+-----------+                 +--------+
```

A very simple server is created that creates the interface `/dev/adder/0` which
would be a single instance accepting requests. It implements a single command
`DCMD_ADDER_INCREMENT` that takes a `uint32_t` value, increments it by one, and
returns the results.

The client will measure how long it takes to do a fixed set of operations. To
support the measurements, the `dev-adder` should support the following options:

- `-n`: number of threads to instantiate
- `-i`: The number for the first instance.

The client has the following options:

- `-n`: number of threads to instantiate
- `-i`: The number of the first instance to connect to
- `-S`: number of samples for the test
- `-I`: Perform an idle test

## 2. Different types of Measurements

### 2.1. Single Threaded Use Case

There should be only one instance of the `dev-adder` running. The client
connects to `/dev/adder/0` and measures how long it takes to do a fixed number
of calls.

```sh
dev-adder -n 1 -i 0 &
waitfor /dev/adder/0
```

Then the client tests with

```sh
client-adder -n 1 -i 0
```

### 2.2. Multiple Server Use Case

There should be a single instance of `dev-adder` running, but it creates
multiple threads, each thread running its own resource manager. The tool should
be configurable on how many threads it sets up:

- `/dev/adder/0`
- `/dev/adder/1`
- `/dev/adder/2`
- ...
- `/dev/adder/7`

Then the client should be configured, one per thread, to talk to a specific
instance.

This test case can show if there is a global lock. The time for incrementing a
fixed number of times for one thread should be the same for n-threads, given n
free cores in the system.

Note, there is one channel per thread, which is different to the QNX
multithreaded resource manager example, which assumes a threadpool servicing a
single path.

### 2.3. Multiple Server Use Case - Process

The same test can be made, but instead of hosting threads in a single server,
host them by starting `dev-adder` n-times. This will show if there is overhead
due to having processes instead of threads.

## 3. Results

There are two modes:

1. When the `dev-adder` is run and it sets up automatically all managers in
   individual threads.

   ```sh
   dev-adder -n6 &
   ```

2. When the `dev-adder` is run multiple times, each process setting up the
   thread.

   ```sh
   dev-adder -n1 -i0 &
   dev-adder -n1 -i1 &
   dev-adder -n1 -i2 &
   dev-adder -n1 -i3 &
   dev-adder -n1 -i4 &
   dev-adder -n1 -i5 &
   ```

The default of 2,000,000 iterations are used.

Then when running the benchmark:

```sh
adder -n$X -I
```

The value of `$X` is set to the number of threads to test.

### 3.1. QNX 8.0 on Raspberry Pi 4

The version tested is:

```sh
# uname -a
QNX rpi4-800-d9c9b4f7 8.0.0 2025/07/30-19:17:34EDT RaspberryPi4B aarch64le
```

Running the software device:

| Mode          | Threads |  Idle | CPU Busy | Ave Duration |            Rate |
| ------------- | ------: | ----: | -------: | -----------: | --------------: |
| One process   |       1 | 0.17% |     129% |      12100ms | 165300 msgs/sec |
|               |       2 | 0.09% |     260% |      15030ms | 133050 msgs/sec |
|               |       3 | 0.11% |     350% |      16030ms | 124770 msgs/sec |
|               |       4 | 0.17% |     400% |      13942ms | 143450 msgs/sec |
|               |       5 | 0.17% |     400% |      21000ms |  95225 msgs/sec |
|               |       6 | 0.09% |     400% |      24430ms |  81870 msgs/sec |
| Multi-process |       1 | 0.09% |     130% |      11999ms | 166681 msgs/sec |
|               |       2 | 0.17% |     259% |      15050ms | 132890 msgs/sec |
|               |       3 | 0.09% |     347% |      16121ms | 124062 msgs/sec |
|               |       4 | 0.17% |     400% |      13542ms | 147690 msgs/sec |
|               |       5 | 0.10% |     400% |      21822ms |  91651 msgs/sec |
|               |       6 | 0.09% |     400% |      26167ms |  76432 msgs/sec |

It is expected that the performance for 5 threads and more decreases as the test
is now CPU bound.

It is interesting however, that at 4 threads, we'd expect the system to be CPU
bound but the performance increases slightly.

### 3.2. QNX 7.1 on Raspberry Pi 4

The version tested is:

```sh
# uname -a
QNX rpi4-710-671c99df 7.1.0 2023/07/14-18:28:39EDT RaspberryPi4B aarch64le
```

Running the software device:

| Mode          | Threads |  Idle | CPU Busy | Ave Duration |            Rate |
| ------------- | ------: | ----: | -------: | -----------: | --------------: |
| One process   |       1 | 0.00% |     100% |       7560ms | 264550 msgs/sec |
|               |       2 | 0.02% |     200% |      15470ms | 129282 msgs/sec |
|               |       3 | 0.02% |     300% |      26744ms |  74783 msgs/sec |
|               |       4 | 0.02% |     400% |      36370ms |  54990 msgs/sec |
|               |       5 | 0.00% |     400% |      48307ms |  41401 msgs/sec |
|               |       6 | 0.00% |     400% |      57902ms |  34541 msgs/sec |
| Multi-process |       1 | 0.00% |     100% |       8068ms | 247893 msgs/sec |
|               |       2 | 0.00% |     200% |      15711ms | 127299 msgs/sec |
|               |       3 | 0.02% |     300% |      27166ms |  73621 msgs/sec |
|               |       4 | 0.00% |     400% |      37386ms |  53496 msgs/sec |
|               |       5 | 0.02% |     400% |      51067ms |  39164 msgs/sec |
|               |       6 | 0.00% |     400% |      61337ms |  32606 msgs/sec |

We see that QNX 7.1 effectively throttles through a single thread, halving
performance for each new thread added. From 5 threads and more, performance is
now spread over multiple cores.
