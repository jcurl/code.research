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

### QNX 8.0

A sample captured from QNX 8.0 on a Rasbperry Pi4 and x86 on QNX 8.0.3.

```txt
# ./time_compare
Time Comparison Tool

Clock                      Steady  Period
-------------------------  ------  ------
High Resolution Clock      Yes     1/1000000000
Steady Clock               Yes     1/1000000000
System Clock               No      1/1000000

QNX Clock Details (pidin syspage=qtime)
- cycles_per_sec:  54000000
- nsec_tod_adjust: 0
- nsec_inc:        1000000
- boot_time:       0
- adjust:          tick_nsec_inc=0; tick_count=0
- interrupt:       27
- boot_cc:         11548565
- tick_period_cc:  54000
- Flags
  [X] QTIME_FLAG_CHECK_STABLE
  [-] QTIME_FLAG_TICKLESS
  [-] QTIME_FLAG_GLOBAL_CLOCKCYCLES

System Clock           Highres Clock          Steady Clock           CLOCK_MONOTONIC        CLOCK_REALTIME         ClockCycles()
---------------------- ---------------------- ---------------------- ---------------------- ---------------------- ----------------------
         418.566008000          418.566008778          418.566009130          418.566009000          418.566009000          418.779872203
         419.567007000          419.567008204          419.567008759          419.567009000          419.567009000          419.780871629
         420.568008000          420.568008519          420.568008870          420.568009000          420.568009000          420.781872092
         421.569009000          421.569009778          421.569010093          421.569010000          421.569010000          421.782873185
         422.570007000          422.570008148          422.570008481          422.570008000          422.570008000          422.783871259
         423.571007000          423.571007926          423.571008278          423.571008000          423.571008000          423.784871277
         424.572011000          424.572012833          424.572013185          424.572013000          424.572014000          424.785876574
         425.573006000          425.573007611          425.573007944          425.573008000          425.573008000          425.786870833
         426.574008000          426.574008815          426.574009019          426.574009000          426.574009000          426.787871851
         427.575008000          427.575008426          427.575008759          427.575009000          427.575009000          427.788871629
         428.576007000          428.576008278          428.576008500          428.576008000          428.576008000          428.789871370
         429.577007000          429.577007611          429.577008037          429.577008000          429.577008000          429.790870888
         430.578007000          430.578007574          430.578007926          430.578008000          430.578008000          430.791870907
```

We see that the all system clocks, except `ClockCycles()` have the same time
base. Only `ClockCycles()` has an offset which is derived from the `boot cc`
parameter.

A `boot cc` of `11548565` is 0.213862315 seconds offset.

Earlier releases of QNX 8.0 (here 8.0.1), with results obtained on September
2024, show slightly different. In older versions, the
`std::chrono::high_resolution_clock` didn't take into about the `boot cc`
parameter and was the same as the `ClockCycles().`

```txt
Time Comparison Tool

Clock                      Steady  Period
-------------------------  ------  ------
High Resolution Clock      Yes     1/1000000000
Steady Clock               Yes     1/1000000000
System Clock               No      1/1000000

QNX Clock Details (pidin syspage=qtime)
- cycles_per_sec:  54000000
- nsec_tod_adjust: 1725824925283890722
- nsec_inc:        1000000
- boot_time:       1725824925
- adjust:          tick_nsec_inc=0; tick_count=0
- interrupt:       27
- boot_cc:         8179745
- tick_period_cc:  54000
- Flags
  [X] QTIME_FLAG_CHECK_STABLE
  [-] QTIME_FLAG_TICKLESS
  [-] QTIME_FLAG_GLOBAL_CLOCKCYCLES

System Clock           Highres Clock          Steady Clock           CLOCK_MONOTONIC        CLOCK_REALTIME         ClockCycles()
---------------------- ---------------------- ---------------------- ---------------------- ---------------------- ----------------------
  1725825006.048897000           80.916483870           80.765007296           80.765007000   1725825006.048898000           80.916484537
  1725825007.049897000           81.917483240           81.766006778           81.766006000   1725825007.049897000           81.917483833
  1725825008.050897000           82.918484203           82.767007537           82.767007000   1725825008.050898000           82.918484703
  1725825009.051898000           83.919484796           83.768008333           83.768008000   1725825009.051899000           83.919485611
  1725825010.052896000           84.920483222           84.769006537           84.769006000   1725825010.052897000           84.920483833
  1725825011.053897000           85.921483296           85.770006630           85.770006000   1725825011.053897000           85.921483703
  1725825012.054896000           86.922482481           86.771005796           86.771006000   1725825012.054896000           86.922482962
  1725825013.055897000           87.923483481           87.772006889           87.772007000   1725825013.055897000           87.923484037
  1725825014.056900000           88.924487388           88.773011000           88.773011000   1725825014.056902000           88.924488333
  1725825015.057898000           89.925484555           89.774007870           89.774008000   1725825015.057898000           89.925484944
  1725825016.058897000           90.926483685           90.775007000           90.775007000   1725825016.058898000           90.926484185
  1725825017.059896000           91.927482833           91.776006241           91.776006000   1725825017.059897000           91.927483425
  1725825018.060896000           92.928482888           92.777006204           92.777006000   1725825018.060897000           92.928483259
  1725825019.061897000           93.929484166           93.778007574           93.778007000   1725825019.061898000           93.929484722
  1725825020.062897000           94.930483777           94.779007111           94.779007000   1725825020.062898000           94.930484185
  1725825021.063897000           95.931484333           95.780007889           95.780008000   1725825021.063898000           95.931485055
```

New in QNX 8.0 is the ability to calculate the difference between the high
resolution clock (`std::chrono::high_resolution_clock` which is based on
`ClockCycles()`) and the steady clock (`std::chrono::steady_clock` which is
based on `CLOCK_MONOTONIC`).

By taking `boot_cc` / `cycles_per_sec`, we calculate the time the bootloader
takes to be 0.151477s, and can see that:

`steady_clock` + (`boot_cc` / `cycles_per_sec`) ~= `high_resolution_clock`.
