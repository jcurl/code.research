# UDP Load <!-- omit in toc -->

This is a tool to stress the network stack for sending packet datagrams over a
network interface.

It can set up multiple threads, configure sending packets with shaping to try
and make the number of packets sent out over a user defined time window be as
constant as possible (with bursts up to 3ms).

One can measure the CPU load the tool takes to measure the overhead in sending
UDP packets. For a micro-kernel system like QNX, it measures the system idle time
to estimate the overhead (specifically the network packet is running as another
user-space program).

- [1. Usage](#1-usage)
  - [1.1. CPU Time Measurements](#11-cpu-time-measurements)
  - [1.2. Low-Level Time Metrics](#12-low-level-time-metrics)
  - [1.3. Multithreaded Behaviour](#13-multithreaded-behaviour)
  - [1.4. Operating Specific Details](#14-operating-specific-details)
    - [1.4.1. Running on Linux](#141-running-on-linux)
      - [1.4.1.1. Understanding the CPU Load Measurements](#1411-understanding-the-cpu-load-measurements)
    - [1.4.2. Running on QNX](#142-running-on-qnx)
      - [1.4.2.1. Understanding the CPU Load Measurements](#1421-understanding-the-cpu-load-measurements)
  - [1.5. Command Line Options and their Usage](#15-command-line-options-and-their-usage)
- [2. Design](#2-design)
- [3. Results](#3-results)

## 1. Usage

A quick overview of the program can be given with the option `-?` on the command
line. It will print the options and give a very brief description of that
option.

### 1.1. CPU Time Measurements

When running, it is advisable to have no other processes running on the system
that could interfere with the load, with a load of as close to as possible, 0%.
For example, on the Raspberry Pi, it's recommended to disable the graphics
system with `raspi-config`.

Important! Disable hyper-threading when making CPU measurements. Many
measurements are based on the assumption the time to run instructions is
independent of which processor is running (i.e. running two threads is twice the
performance of running one thread, which is not true for hyper-threading).

On output, two CPU load measurements are provided:

- Total CPU Busy
- Process CPU Busy

The "Total CPU Busy" is the measured idle time in the system, subtracted from
the total running time of the system. So if the system has four cores, and it is
completely busy, the total Run time is 400%.

The "Total CPU Busy" is the measured busy time for the process running.

On Linux, the "Process CPU Busy" is usually what is interesting to analyse, as
it accounts for time the process runs in user space and in the Kernel. The
"Total CPU Busy" should report extra CPU the Linux kernel runs (e.g. due to
interrupt handling), but the Linux documentation states some obvious pitfalls
with regards to how idle time is measured and may be inaccurate.

On QNX, one must measure all processes (and as such, IDLE should be 0%) and the
"Total CPU Busy" is what is interesting.

### 1.2. Low-Level Time Metrics

Two outputs:

- Time in send
- Time in sleep

Describes the number of milliseconds the threads were sending data, and the
number of milliseconds in a system `sleep()` call. This helps to correlate to
the CPU loads and how much time is spent blocking (e.g. waiting for hardware
interrupts when sending traffic). For example, if the CPU idle time is high in
comparison to the Time in send/sleep, then the idle time is in socket system
calls.

It can effectively give information on the time the test process is in I/O
blocking. Note, it is the sum of all time in threads, running more threads than
cores in the system will produce non-meaningful results.

### 1.3. Multithreaded Behaviour

when more than one thread is chosen on the command line, each thread sets up its
own independent socket (with `SO_REUSEADDR` and `SO_REUSEPORT`). There is no
synchronisation between threads.

### 1.4. Operating Specific Details

#### 1.4.1. Running on Linux

##### 1.4.1.1. Understanding the CPU Load Measurements

The "Total CPU Busy" is calculated by the idle time presented in `/proc/stat`
for the first line which is the CPU. Linux documentation states that this
parameter only samples at the time of a jiffy if a process is running or not.
For processes that run at periodic intervals of the Jiffy interval, results are
distorted. Hence, do not use the "Total CPU" measurement on Linux.

To have a more accurate measure of the overall idle time (and by deduction the
total busy time), it is recommended to use other system tools, like `perf`, and
configuring the scheduler to have a fixed frequency, so that the number of busy
cycles can be obtained.

The other value, "Process CPU busy" is much more accurate, but measures only the
CPU time in user-space and system-calls for this program (combined). This is
similar to running the "time" command. For Linux, this is a reasonable
measurement, as it measures the time the Kernel spends doing network operations.

#### 1.4.2. Running on QNX

##### 1.4.2.1. Understanding the CPU Load Measurements

QNX, being a microkernel system, doesn't run the network stack in kernel space
within the context of the process. It is not sufficient to only measure the CPU
time of this program running, as it doesn't take into account the separate
process running the network stack and drivers.

For this, the "Total CPU Busy" is important and can be accurately measured. The
idle time is accurately measured on QNX (by measuring the time in each idle
thread).

The "Process CPU Busy" is the time only spent in the program (when not waiting
on the network stack).

### 1.5. Command Line Options and their Usage

The tool works by sending out UDP IPv4 packets over regular intervals, measured
using the high resolution clock in the system. It uses a shaper over a window
that can be specified to smooth traffic, at the same time to allow for some
jitter in the system.

For help, see `udp_load -?`:

```
$ udp_load -?
udp_load [-n<slots>] [-m<width>] [-p<packets>] [-s<size>]
  [-d<duration>] [-T<threads>] [-I] -S<sourceip> -D<destip>

Writes UDP packets bound from <sourceip> IPv4 address to <destip>.

 -B<mode> - sending mode: sendto; sendmmsg. Default is 'sendto'
 -n<slots> - Number of slots in a time window (default 20).
 -m<width> - Width of each slot in milliseconds (default 5ms).
 -p<packets> - Number of packets to send in a window (n*m duration, default 1000).
 -s<size> - Size of each UDP packet (default 1472).
 -d<duration> - Duration to run the test in milliseconds (default 30,000ms).
 -T<threads> - Number of parallel threads (default 1).
 -S<sourceip> - Source IP address (must be an existing interface).
 -D<destip> - Destination IP address (can be unicast or multicast).
 -I - Enable IDLE mode test prior

 -? - Display this help.
```

A window of traffic is defined by a duration of time of `n` slots, over `m`
milliseconds per slot. So if the default of `n=20` and `m=5`, then the window
for shaping is 100ms.

The shaper tries to send a constant rate of `p` packets in the time window
given. It divides `p` by the number of slots `n` and sends them every `m`
milliseconds.

The size of each packet is defined by `s`, which is default 1472 bytes per UDP
packet. This corresponds to 1500 bytes of payload (MTU) of an Ethernet packet.
One could test based on size, an efficient driver implementation should have a
CPU load proportional to the number of packets per second, irrespective of their
size.

The test should run for `d` duration milliseconds. This is a guideline, and may
be rounded. The actual time is calculated and printed at the end, and is
dependent on the width of a slot, `m`.

To allow tests for multithreaded implementations, the number of sending threads
can be specified with the `T` option. Increasing the number of threads can show
if there are effects of serialization with independent threads running in the
system. It can be observed if the I/O bandwidth can be increased by increasing
the number of threads. The packets send per thread per slot is `p / n / T`, that
is, it still tries to send only `p` packets per window, scaling down the number
of packets it sends per slot per thread.

The source and destination addresses are mandatory. The source `-S` is the
address of the interface to send *from*. Usually the tool `ifconfig` is used to
identify an assigned IP address to an interface and this is used. Append with a
colon and the port number to bind to (e.g. `127.0.0.1:3499`). The destination
address is either a unicast or a multicast address to send to. Specify the port
to send to with a colon and the port number.

The `-I` option instructs to run a 5s test in idle, just measuring the CPU busy
time when nothing should occur. This can give an indication of work being done
outside of the context of the test being run.

The `-B` chooses how the packets are sent:

- `sendto` uses the `sendto()` call sending one packet at a time.
- `sendmmsg` uses the `sendmmsg()` call sending up to 1024 packets in one slot.

## 2. Design

The design of the shaper is given in the [Jupyter
Notebook](./udp_load/doc/shaper.ipynb). It is written in Python so that
different scenarios can be created interactively and observed how the shaping
algorithm behaves. This shaper algorithm is then translated into C++ code in the
file [`udp_talker.cpp`](./udp_load/udp_talker.cpp).

## 3. Results

To see a sample of results, see [Results](./udp_load/results/results.md).

Use the [Jupyter Notebook graph.ipynb](./udp_load/results/graph.ipynb) to
explore and plot the data.
