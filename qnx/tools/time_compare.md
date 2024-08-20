# Time Compare

A simple tool, that prints out the various clocks on the console at regular
periods. The test can show empirically the source of a C++ clock (by comparing
against POSIX clocks). If two clocks appear to be the same, then running it over
a longer period of time could show drift between the clocks.

## Observations

### Linux 6.8.0

```text
Time Comparison Tool

Clock                      Steady  Period
-------------------------  ------  ------
High Resolution Clock      No      1/1000000000
Steady Clock               Yes     1/1000000000
System Clock               No      1/1000000000

System Clock           Highres Clock          Steady Clock           CLOCK_MONOTONIC        CLOCK_REALTIME
---------------------- ---------------------- ---------------------- ---------------------- ----------------------
  1724149953.141134541   1724149953.141134951         1000.209880459         1000.209880734   1724149953.141135797
  1724149954.141584729   1724149954.141584834         1001.210329995         1001.210330114   1724149954.141585103
  1724149955.141884151   1724149955.141884323         1002.210629577         1002.210629729   1724149955.141884789
```

### QNX 7.1

A sample output captured on a QEMU Virtual Machine running from Linux:

```txt
Time Comparison Tool

Clock                      Steady  Period
-------------------------  ------  ------
High Resolution Clock      Yes     1/1000000000
Steady Clock               Yes     1/1000000000
System Clock               No      1/1000000

QNX Clock Details (pidin syspage=qtime)
- cycles_per_sec:  2899996100
- nsec_tod_adjust: 1723449063526000000
- nsec:            364464048154
- nsec_inc:        1000000
- boot_time:       1723449064
- adjust:          tick_nsec_inc=-4740000; tick_count=0
- interrupt:       2
- timer_prog_time: 0
- Flags
  [-] QTIME_FLAG_TIMER_ON_CPU0
  [X] QTIME_FLAG_CHECK_STABLE
  [-] QTIME_FLAG_TICKLESS
  [-] QTIME_FLAG_TIMECC
  [-] QTIME_FLAG_GLOBAL_CLOCKCYCLES

System Clock           Highres Clock          Steady Clock           CLOCK_MONOTONIC        CLOCK_REALTIME         ClockCycles()
---------------------- ---------------------- ---------------------- ---------------------- ---------------------- ----------------------
  1723449428.997048000          365.927558185          365.471048154          365.471048000   1723449428.997048000          365.927568436
  1723449429.998048000          366.928550337          366.472048154          366.472048000   1723449429.998048000          366.928552926
  1723449430.999048000          367.929590103          367.473048154          367.473048000   1723449430.999048000          367.929594700
  1723449432.000048000          368.930543436          368.474048154          368.474048000   1723449432.000048000          368.930545601
```

The following observations can be made:

- the System Clock (`std::chrono::system_clock`) is the same as `CLOCK_REALTIME`.
- The Highres clock (`std::chrono:high_resolution_clock`) is monotonic, but it
  is not the same as `CLOCK_MONOTONIC`, instead derived from `ClockCycles()`.
  There is an interesting difference (documented by QNX), that the
  `CLOCK_MONOTONIC` and the `ClockCycles()` may drift, unless `libmod_timecc`
  module is loaded that keeps the two clocks synchronised.
- The Linux Highres Clock (based on `CLOCK_REALTIME`) and QNX Highres Clock
  (based on `ClockCycles()`) have different time bases, Linux mapping to UTC
  time since 1/1/1970, and QNX based on a Time Stamp Counter register that runs
  from when the processor first boots.
