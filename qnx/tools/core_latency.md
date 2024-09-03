# Core Latency <!-- omit in toc -->

Measure the cache latency between two cores on the system (NUMA, multicore). The
tool only works on systems with at least two threads (hyperthreading, or cores,
or processors). See your processor's technical datasheet for the topology of the
tool. On Intel systems, this can usually be obtained by analysing the CPUID
information on each processor (see also my tool
[CPUID](https://github.com/jcurl/RJCP.DLL.CpuId/blob/master/README.md) for
Windows).

It is inspired by the rust tool [core-to-core
latency](https://github.com/nviennot/core-to-core-latency) ported to C++ so it
can be tested on other Operating Systems (notably QNX).

- [1. Usage](#1-usage)
  - [1.1. CAS Latency Test](#11-cas-latency-test)
  - [1.2. Load/Store or Read/Write latency test](#12-loadstore-or-readwrite-latency-test)
- [2. Design](#2-design)
  - [2.1. CAS Latency](#21-cas-latency)
    - [2.1.1. Disassembly in C++ for x86\_64](#211-disassembly-in-c-for-x86_64)
    - [2.1.2. Optimisations for x86\_64](#212-optimisations-for-x86_64)
  - [2.2. Load/Store](#22-loadstore)
    - [2.2.1. Disassembly in C++ for x86\_64](#221-disassembly-in-c-for-x86_64)
- [3. Results](#3-results)
  - [3.1. Intel i9-13980HX with P and E cores](#31-intel-i9-13980hx-with-p-and-e-cores)
    - [3.1.1. Single Line CAS](#311-single-line-cas)
    - [3.1.2. Two Line Read/Write Ping/Pong](#312-two-line-readwrite-pingpong)
  - [3.2. Intel i3-2120T](#32-intel-i3-2120t)
    - [3.2.1. Single Line CAS](#321-single-line-cas)
    - [3.2.2. Two Line Read/Write Ping/Pong](#322-two-line-readwrite-pingpong)

## 1. Usage

Run `core_latency -?` for information.

To run CAS latency tests:

```sh
core_latency -b cas
```

To run Load/Store latency tests:

```sh
core_latency -b readwrite
```

### 1.1. CAS Latency Test

This test does a Compare and Swap on two threads. One thread swaps when the
value is PING (writing PONG), the other thread swaps when the value is PONG
(writing PING). The reads and writes are on the same cache-line.

### 1.2. Load/Store or Read/Write latency test

The test has two threads, each swapping state on change, moving data from one
thread to another thread using atomic load and store operations.

## 2. Design

### 2.1. CAS Latency

The CAS (Compare and Swap) latency is a ping-pong system where one core waits to
switch from the value ping to pong, and another core wats to swith from pong to
ping. This measures the delays in the synchronisation of the caches between the
two cores.

#### 2.1.1. Disassembly in C++ for x86_64

Using `objdump -d` to view the assembly for the "ping" thread:

```cpp
for (std::uint32_t i = 0; i < iterations_ * samples_; i++) {
  std::uint32_t expected_flag = PING;
  do {
    expected_flag = PING;
  } while (!flag_.compare_exchange_strong(
      expected_flag, PONG, std::memory_order::memory_order_relaxed,
      std::memory_order::memory_order_relaxed));
}
```

In the assembly: "The ZF flag is set if the values in the destination operand
and register AL, AX, or EAX are equal; otherwise it is cleared". That is, the
Zero-Flag is set if there was a swap.

```asm
    334b:       31 c9                   xor    %ecx,%ecx
    334d:       31 ff                   xor    %edi,%edi
    334f:       be 01 00 00 00          mov    $0x1,%esi
    3354:       0f 1f 40 00             nopl   0x0(%rax)
    3358:       89 f8                   mov    %edi,%eax

    // Compares EAX (PING=0) against `_flag`, if equal, copy ESI (PONG=1)
    335a:       f0 0f b1 72 08          lock cmpxchg %esi,0x8(%rdx)
    335f:       75 14                   jne    3375 <_ZNSt6thread11_State_implINS_8_InvokerISt5tupleIJZN14core_benchmark3runEjjEUlvE_EEEEE6_M_runEv+0x55>

    // Increment loop
    3361:       48 8b 53 18             mov    0x18(%rbx),%rdx
    3365:       83 c1 01                add    $0x1,%ecx
    3368:       8b 42 0c                mov    0xc(%rdx),%eax
    336b:       0f af 42 10             imul   0x10(%rdx),%eax
    336f:       39 c1                   cmp    %eax,%ecx
    3371:       72 e5                   jb     3358 <_ZNSt6thread11_State_implINS_8_InvokerISt5tupleIJZN14core_benchmark3runEjjEUlvE_EEEEE6_M_runEv+0x38>
    3373:       5b                      pop    %rbx
    3374:       c3                      ret

    // CAS failed, reload value and try again
    3375:       48 8b 53 18             mov    0x18(%rbx),%rdx
    3379:       eb dd                   jmp    3358 <_ZNSt6thread11_State_implINS_8_InvokerISt5tupleIJZN14core_benchmark3runEjjEUlvE_EEEEE6_M_runEv+0x38>
```

The other loop:

```cpp
for (std::uint32_t i = 0; i < samples_; i++) {
  auto start = std::chrono::high_resolution_clock::now();
  for (std::uint32_t j = 0; j < iterations_; j++) {
    std::uint32_t expected_flag = PONG;
    do {
      expected_flag = PONG;
    } while (!flag_.compare_exchange_strong(
        expected_flag, PING, std::memory_order::memory_order_relaxed,
        std::memory_order::memory_order_relaxed));
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::uint32_t duration = std::chrono::nanoseconds(end - start).count();
  total_time += duration;
```

with disassembly:

```asm
    33d8:       44 89 f0                mov    %r14d,%eax
    33db:       f0 0f b1 6a 08          lock cmpxchg %ebp,0x8(%rdx)
    33e0:       75 32                   jne    3414 <_ZNSt6thread11_State_implINS_8_InvokerISt5tupleIJZN14core_benchmark3runEjjEUlvE0_EEEEE6_M_runEv+0x94>

    // Update loop
    33e2:       48 8b 53 18             mov    0x18(%rbx),%rdx
    33e6:       83 c1 01                add    $0x1,%ecx
    33e9:       3b 4a 0c                cmp    0xc(%rdx),%ecx
    33ec:       72 ea                   jb     33d8 <_ZNSt6thread11_State_implINS_8_InvokerISt5tupleIJZN14core_benchmark3runEjjEUlvE0_EEEEE6_M_runEv+0x58>

    // Time loop
    33ee:       e8 8d ee ff ff          call   2280 <_ZNSt6chrono3_V212system_clock3nowEv@plt>
    33f3:       48 8b 53 20             mov    0x20(%rbx),%rdx
    33f7:       41 83 c5 01             add    $0x1,%r13d
    33fb:       44 29 e0                sub    %r12d,%eax
    33fe:       48 01 02                add    %rax,(%rdx)
    3401:       48 8b 43 18             mov    0x18(%rbx),%rax
    3405:       44 3b 68 10             cmp    0x10(%rax),%r13d
    3409:       72 b5                   jb     33c0 <_ZNSt6thread11_State_implINS_8_InvokerISt5tupleIJZN14core_benchmark3runEjjEUlvE0_EEEEE6_M_runEv+0x40>
    340b:       5b                      pop    %rbx
    340c:       5d                      pop    %rbp
    340d:       41 5c                   pop    %r12
    340f:       41 5d                   pop    %r13
    3411:       41 5e                   pop    %r14
    3413:       c3                      ret

    // CAS failed, reload value and try again
    3414:       48 8b 53 18             mov    0x18(%rbx),%rdx
    3418:       eb be                   jmp    33d8 <_ZNSt6thread11_State_implINS_8_InvokerISt5tupleIJZN14core_benchmark3runEjjEUlvE0_EEEEE6_M_runEv+0x58>
```

and in Rust:

```txt
   25f40:       b0 01                   mov    $0x1,%al
   25f42:       f0 45 0f b0 77 28       lock cmpxchg %r14b,0x28(%r15)
   25f48:       75 f6                   jne    25f40 <_ZN4core3ops8function6FnOnce40call_once$u7b$$u7b$vtable.shim$u7d$$u7d$17h6c94826e75d0958aE+0x170>
   25f4a:       ff c1                   inc    %ecx
   25f4c:       39 d9                   cmp    %ebx,%ecx
   25f4e:       75 f0                   jne    25f40 <_ZN4core3ops8function6FnOnce40call_once$u7b$$u7b$vtable.shim$u7d$$u7d$17h6c94826e75d0958aE+0x170>
   25f50:       48 8b 7c 24 08          mov    0x8(%rsp),%rdi
                    for _ in 0..num_round_trips {
                        while state.flag.compare_exchange(PONG, PING, Ordering::Relaxed, Ordering::Relaxed).is_err() {}
                    }
                    let end = clock.raw();
   25f55:       ff 15 7d e0 0d 00       call   *0xde07d(%rip)        # 103fd8 <_GLOBAL_OFFSET_TABLE_+0xc30>
```

#### 2.1.2. Optimisations for x86_64

This results in a hand-written optimised version in assembly.

```c++
template <std::uint32_t compare, std::uint32_t swap>
__attribute__((always_inline)) auto cas(std::atomic<std::uint32_t> &flag)
    -> void {
#if __x86_64__
  asm volatile(
      " mov %1,%%esi;"
      "0:"
      " mov %0,%%eax;"
      " lock cmpxchg %%esi,%2;"
      " jne 0b;"
      :
      : "i"(compare), "i"(swap), "m"(flag)
      : "eax", "esi");
#else
#endif
}
```

One must be careful when providing the compiler flags, that additional
instructions are not added in the for loop. Some compiler options provide `nop`
which distorts the results.

### 2.2. Load/Store

The basic principle is to have two variables, each in their own cache-line
separated so a memory access must read from the cache line. This requires an
alignment of 128 bytes on Intel and ARM processors.

Thread pong:

- [ X ] -> [ X ] : ping == PONG (wait for ping == PING)
- [ X ] -> [ Y ] : ping == PING; pong = PONG
- [ Y ] -> [ Y ] : ping == PING (wait for ping == PONG)
- [ Y ] -> [ X ] : ping == PONG; pong = PING

Thread ping:

- [ X' ] -> [ X' ] : pong == PING (wait for pong == PONG)
- [ X' ] -> [ Y' ] : pong == PONG; ping = PONG
- [ Y' ] -> [ Y' ] : pong == PONG (wait for pong == PING)
- [ Y' ] -> [ X' ] : pong == PING; ping = PING

For the sequence, it measures the time between state changes. The threads
serialise each other in the following sequence. To ensure no deadlocks, initial
conditions require that `ping = PING` and `pong = PING`.

| Ping Thread           | Pong Thread           |
| --------------------- | --------------------- |
| Wait for pong == PONG | Wait for ping == PING |
|                       | pong = PONG           |
|                       | Wait for ping == PONG |
| ping = PONG           |                       |
|                       | pong = PING           |
| Wait for pong == PING |                       |
| ping = PING           |                       |

If the initial conditions are incorrect:

- `ping = PONG`; `pong = X`; The pong thread won't run.
- `ping = PING`; `pong = PONG`; If the ping thread runs first, the pong thread
  won't run (race condition).
- `ping = PING`; `pong = PING`; The two threads are always toggling between each
  other.

#### 2.2.1. Disassembly in C++ for x86_64

Under Intel, the generated assemlby is very simple due to the memory ordering
model of Intel processors. Compiling with `-g` option and disassembly shows:

```sh
objdump -dSC ./tools/core_latency/core_latency | less
```

with the following code:

```cpp
std::uint32_t v = PING;
for (std::uint32_t i = 0; i < iterations_ * samples_; i++) {
  while (this->ping_.load(std::memory_order::memory_order_acquire) != v) {
  }
  this->pong_.store(!v, std::memory_order::memory_order_release);
  v = !v;
}
```

and disassembled to:

```asm
    38cc:       8b 80 80 00 00 00       mov    0x80(%rax),%eax
      while (this->ping_.load(std::memory_order::memory_order_acquire) != v) {
    38d2:       39 d0                   cmp    %edx,%eax
    38d4:       75 f2                   jne    38c8 <std::thread::_State_impl<std::thread::_Invoker<std::tuple<corerw_benchmark::run(unsigned int, unsigned int)::{lambda()#1}> > >::_M_run()+0x38>
        __atomic_store_n(&_M_i, __i, int(__m));
    38d6:       48 8b 43 18             mov    0x18(%rbx),%rax
    38da:       83 f2 01                xor    $0x1,%edx
    for (std::uint32_t i = 0; i < iterations_ * samples_; i++) {
    38dd:       83 c6 01                add    $0x1,%esi
    38e0:       89 90 00 01 00 00       mov    %edx,0x100(%rax)
    38e6:       48 8b 43 18             mov    0x18(%rbx),%rax
    38ea:       8b 48 08                mov    0x8(%rax),%ecx
    38ed:       0f af 48 0c             imul   0xc(%rax),%ecx
    38f1:       39 ce                   cmp    %ecx,%esi
    38f3:       72 d7                   jb     38cc <std::thread::_State_impl<std::thread::_Invoker<std::tuple<corerw_benchmark::run(unsigned int, unsigned int)::{lambda()#1}> > >::_M_run()+0x3c>
```

The value at `0x80(%rax)` is `alignas(128) std::atomic<std::uint32_t> ping_;`.
This shows the atomic load on Intel is just a `mov` instruction. The core of the
loop is then a `mov` and a `cmp` instruction at 32-bits.

## 3. Results

### 3.1. Intel i9-13980HX with P and E cores

#### 3.1.1. Single Line CAS

The core-to-core for a single line (CAS) latencies. Turbo boost is disabled
under Linux by:

```sh
echo "1" > /sys/devices/system/cpu/intel_pstate/no_turbo
```

When testing between Core X and Y, it shouldn't matter which core is the ping,
which is the pong, they should have the same values. The time is the average of
all the samples, and is half the time of a ping-pong-ping.

```txt
Running Core Benchmark
 Samples: 500
 Iterations: 4000

      0     1     2     3     4     5     6     7     8     9     10    11    12    13    14    15    16    17    18    19    20    21    22    23    24    25    26    27    28    29    30    31
0           10    98    98    97    97    94    94    93    93    94    94    93    93    90    90    96    96    96    96    94    94    94    94    88    88    88    88    106   106   106   106
1     9           98    98    97    97    94    94    93    93    94    94    93    93    90    90    96    96    96    96    94    94    94    94    88    88    88    88    106   106   106   106
2     98    98          9     98    98    95    95    94    94    95    95    94    94    91    91    96    96    96    96    96    96    96    96    83    83    83    83    105   105   105   105
3     98    98    9           98    98    95    95    94    94    95    95    94    94    91    91    96    96    96    96    96    96    96    96    83    83    83    83    105   105   105   105
4     96    96    98    98          9     94    94    93    93    94    94    93    93    90    90    95    95    95    95    94    94    94    94    83    83    83    83    103   103   103   103
5     96    96    98    98    9           94    94    93    93    94    94    93    93    90    90    95    95    95    95    94    94    94    94    83    83    83    83    103   103   103   103
6     94    94    95    95    94    94          9     90    90    92    92    90    90    88    88    93    93    93    93    92    92    92    92    86    86    86    86    103   103   103   103
7     94    94    95    95    94    94    9           90    90    91    92    90    90    88    88    93    93    93    93    92    92    92    92    86    86    86    86    103   103   103   103
8     93    93    94    94    93    93    90    90          9     90    90    89    89    87    86    92    92    92    92    90    90    90    90    85    85    85    85    102   102   102   102
9     93    93    94    94    93    93    90    90    9           90    90    89    89    87    87    92    92    92    92    90    90    90    90    85    85    85    85    102   102   102   102
10    94    94    95    95    94    94    91    91    90    90          9     90    90    88    88    93    93    93    93    92    92    92    92    80    80    80    80    100   100   100   100
11    94    94    95    95    94    94    91    91    90    90    9           90    90    88    88    93    93    93    93    92    92    92    92    80    80    80    80    100   100   100   100
12    93    93    94    94    93    93    90    90    89    89    90    90          9     86    86    91    91    91    91    90    90    90    90    80    80    80    80    98    98    98    98
13    93    93    94    94    93    93    90    90    89    89    90    90    9           86    86    91    91    91    91    90    90    90    90    80    80    80    80    98    98    98    98
14    90    90    91    92    90    90    88    88    87    87    88    88    87    87          9     89    89    89    89    87    87    87    87    81    81    81    81    100   100   100   100
15    90    90    92    92    90    90    88    88    87    87    88    88    87    86    9           89    89    89    89    87    87    87    87    81    81    81    81    100   100   100   100
16    96    96    96    96    95    95    93    93    92    92    93    93    91    91    89    89          135   135   135   131   130   131   130   110   110   110   110   145   146   146   145
17    96    96    96    96    95    95    93    93    92    92    93    93    91    91    89    89    135         135   133   130   130   130   130   110   110   110   110   145   145   145   145
18    96    96    96    96    95    95    93    93    92    92    93    93    91    91    89    89    134   135         134   131   130   131   129   109   110   110   110   146   146   145   146
19    96    96    96    96    95    95    93    93    92    92    93    93    91    91    89    89    135   135   135         130   130   130   130   109   110   110   110   145   145   145   146
20    94    94    96    96    94    94    92    92    90    90    92    92    90    90    87    87    130   130   130   130         135   135   135   107   107   107   107   143   143   144   143
21    94    94    96    96    94    94    92    92    90    90    92    92    90    90    87    87    130   130   131   130   135         135   135   106   107   107   107   143   144   143   143
22    94    94    96    96    94    94    92    92    90    90    92    92    90    90    87    87    130   131   130   130   135   135         136   107   108   107   107   143   144   143   143
23    94    94    96    96    94    94    92    92    90    90    92    92    90    90    87    87    130   131   131   131   135   135   135         108   107   107   107   143   144   143   143
24    88    88    83    83    83    83    86    86    85    85    80    80    80    80    81    81    110   110   110   110   108   107   108   107         114   114   114   123   123   123   123
25    88    88    83    83    83    83    86    86    85    85    80    80    80    80    81    81    110   110   110   110   108   107   108   108   114         114   114   123   123   123   123
26    88    88    83    83    83    83    86    86    85    85    80    80    80    80    81    81    109   110   110   110   107   108   108   108   114   114         114   123   123   123   123
27    88    88    83    83    83    83    86    86    85    85    80    80    80    80    81    81    110   110   110   110   107   108   108   108   114   114   114         123   122   123   123
28    106   106   105   105   103   103   104   104   102   102   100   100   98    98    100   100   145   146   146   146   144   144   144   143   123   123   123   123         162   162   161
29    106   106   105   105   103   103   104   104   102   102   100   100   98    98    100   100   146   146   145   146   143   144   144   141   123   122   123   123   162         162   162
30    106   106   105   105   103   103   104   104   102   102   100   100   98    98    100   100   146   146   146   145   144   144   144   144   123   123   122   123   162   162         162
31    106   106   105   105   103   103   104   104   102   102   100   100   98    98    100   100   146   146   146   146   144   143   144   144   123   123   123   123   161   162   157
```

In these results we can clearly see:

- Cores 0-15 are the P-Cores and there is hyperthreading enabled
  - Hyperthreading shows approximately 10ns between cores (0-1, 2-3, 4-5. 6-7.
    8-9. 10-11, 12-13. 14-15).
- P-core to P-core is normally 90-100ns latency
- E-core to E-core is 110-150ns latency

When running the test multiple times for the same binaries, we also observe:

- The latency on hyperthreaded cores remains about 10ns
- The Core to Core varies for the same core between 70-105ns, but is consistent
  for that particular run.

Running a version of the tool written for
[Rust](https://github.com/nviennot/core-to-core-latency) (output modified
slightly to make it more compact):

```txt
CPU: 13th Gen Intel(R) Core(TM) i9-13980HX
Num cores: 32
Num iterations per samples: 4000
Num samples: 500

1) CAS latency on a single shared cache line

           0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22    23    24    25    26    27    28    29    30    31
      0
      1   11
      2   94    94
      3   94    94    10
      4   93    93    93    92
      5   89    92    93    93    10
      6   91    91    91    91    90    90
      7   91    91    91    91    90    90    10
      8   93    93    93    93    92    92    90    90
      9   93    93    93    93    92    92    90    90    12
     10   90    90    90    90    89    89    88    88    87    87
     11   90    90    90    90    89    89    88    88    87    87    10
     12   91    91    91    92    91    91    89    89    88    88    88    88
     13   91    92    92    92    91    90    89    89    88    88    88    88    12
     14   87    87    87    87    86    86    84    84    83    83    84    84    82    82
     15   87    87    87    87    86    86    84    84    83    83    83    84    82    82    10
     16   82    82    82    82    79    79    79    79    78    78    78    78    75    76    75    75
     17   82    82    82    82    79    79    79    79    78    78    78    78    75    75    75    75   114
     18   82    82    82    82    79    79    79    79    78    78    78    78    75    75    75    75   114   113
     19   82    82    82    82    79    79    79    79    78    78    78    78    76    76    75    75   114   113   113
     20  101   101   100   100    99    99    99    99    97    97    97    97    96    96    95    95   117   117   117   116
     21  101   101   100   100   100    99    99    99    97    97    97    97    96    95    95    95   117   117   117   117   151
     22  101   101   100   100    99    99    98    98    97    97    97    97    96    96    95    95   117   117   117   116   151   152
     23  101   101   100   100    99    99    99    99    97    97    97    97    96    96    95    95   117   116   116   116   152   151   152
     24  105   105   104   104   102   102   101   101   101   101   100   100    99    99    98    98   121   121   121   122   153   153   153   153
     25  105   105   104   104   102   102   101   101   101   101   100   100    99    99    98    98   121   121   121   121   153   153   153   153   162
     26  105   105   104   104   102   102   102   101   101   101   100   100    99    99    98    98   121   121   121   121   153   153   153   153   162   162
     27  105   105   104   104   102   102   101   101   101   101   100   100    99    99    98    98   121   122   121   121   152   151   153   152   159   158   162
     28  104   104   104   104   103   103   102   102   101   101   100   100    99    99    98    98   121   121   121   121   153   153   153   153   158   158   158   158
     29  104   104   104   104   103   103   102   102   101   101   100   100    99    99    98    98   122   122   122   122   154   154   154   154   159   158   158   158   164
     30  104   104   104   104   103   103   102   102   101   101   100   100    99    99    98    98   122   121   121   122   153   153   153   153   158   158   158   155   164   164
     31  104   104   104   104   102   103   102   102   101   101   100   100    99    99    98    98   121   121   120   121   152   153   153   153   158   157   158   158   163   164   164

    Min  latency: 10.0ns ±0.0 cores: (3,2)
    Max  latency: 163.9ns ±0.2 cores: (29,28)
    Mean latency: 102.9ns
```

It shows similar results:

- Hyperthreading shows 10ns in Rust, 20ns in C++.
- Slightly faster speeds on cores 0-15 with cores 16-19
- Cores 16-19 appear to be faster also

#### 3.1.2. Two Line Read/Write Ping/Pong

```text
Running Read/Write Core Benchmark
 Samples: 500
 Iterations: 4000

      0     1     2     3     4     5     6     7     8     9     10    11    12    13    14    15    16    17    18    19    20    21    22    23    24    25    26    27    28    29    30    31
0           33    127   128   134   126   140   129   136   155   122   121   120   119   116   116   141   141   141   141   142   142   142   142   148   148   149   148   147   148   147   148
1     33          127   128   127   126   140   130   136   156   123   121   120   119   131   116   141   141   141   141   142   142   142   142   148   148   148   148   147   148   147   148
2     137   138         33    144   171   198   171   160   156   130   169   151   151   155   116   193   142   142   143   194   154   193   145   150   204   151   157   147   202   202   158
3     139   136   33          144   140   183   163   147   162   133   134   173   151   148   116   155   143   143   193   188   163   159   147   151   203   154   148   147   164   149   148
4     176   175   165   149         33    140   130   168   171   135   136   118   125   169   142   170   139   142   142   191   191   191   187   148   200   200   200   198   157   146   199
5     136   138   144   130   33          135   168   152   156   128   130   137   128   127   126   160   143   170   160   152   142   186   142   191   152   149   149   153   164   146   146
6     131   131   125   124   127   122         33    134   156   122   157   168   152   170   153   144   139   138   186   189   189   149   142   198   198   198   186   171   146   144   196
7     130   148   127   133   158   122   33          132   167   125   126   120   153   127   113   140   137   165   139   186   141   165   188   175   145   159   148   176   144   158   145
8     141   151   164   164   152   148   127   159         33    142   164   157   132   136   132   156   172   172   158   124   125   129   126   185   185   155   134   151   169   182   182
9     135   133   133   137   153   161   141   132   33          135   153   157   127   114   132   166   129   155   126   147   175   130   127   160   156   132   132   162   133   132   132
10    136   126   131   128   133   121   130   122   128   130         33    107   159   113   110   162   161   176   176   173   174   153   126   131   132   136   183   186   148   135   185
11    131   129   165   128   128   128   142   121   134   130   33          110   105   113   117   141   125   141   134   131   129   129   130   136   133   150   158   132   132   139   141
12    145   145   164   149   159   143   151   139   131   122   130   126         33    145   123   155   155   155   156   154   158   158   158   167   167   167   167   167   167   164   165
13    158   158   169   151   163   148   151   151   119   122   142   129   33          135   133   157   157   157   157   158   158   153   155   161   161   164   165   167   166   164   164
14    156   156   155   154   173   153   142   149   139   147   157   159   146   129         33    184   183   183   183   181   168   174   173   177   177   177   177   193   194   180   179
15    123   133   155   155   176   153   149   148   138   148   114   159   146   130   33          183   183   181   170   174   167   171   177   192   193   194   190   194   194   180   180
16    177   177   189   175   197   172   170   170   169   170   182   171   168   152   156   153         148   147   146   248   245   247   246   261   262   264   261   261   261   261   261
17    177   177   200   175   197   172   181   170   167   170   181   170   157   152   173   153   147         147   146   248   248   248   248   263   265   265   263   264   264   265   265
18    177   177   175   175   172   172   195   170   168   170   177   170   172   152   174   153   148   148         148   251   252   251   251   266   266   267   266   267   266   266   266
19    177   177   196   175   197   172   194   170   169   170   181   171   152   152   161   153   149   149   149         254   252   255   254   270   270   269   270   270   270   270   269
20    176   176   200   174   191   171   174   169   166   166   174   163   160   157   176   160   254   248   248   248         146   146   146   261   262   263   267   260   268   266   267
21    176   176   174   174   185   171   194   169   166   166   168   162   174   157   173   159   253   255   254   256   149         149   149   270   263   269   270   270   270   268   270
22    176   176   185   173   196   171   182   169   166   166   180   162   157   157   162   159   255   252   251   252   148   148         148   267   266   267   267   267   267   267   268
23    176   176   200   173   184   170   172   169   166   166   170   163   159   158   176   159   244   247   254   254   148   147   146         265   263   265   262   261   262   262   262
24    184   184   205   184   189   181   206   179   176   176   190   171   184   166   186   168   267   266   268   267   267   265   268   268         168   168   168   283   283   284   284
25    185   185   211   184   187   181   205   179   176   175   190   171   184   166   186   168   269   267   268   267   269   268   266   268   167         167   169   283   282   283   284
26    184   185   187   185   182   181   179   179   175   175   190   171   184   166   186   168   270   269   270   271   271   271   270   271   169   169         169   285   284   285   285
27    184   185   186   184   190   181   180   179   176   176   190   171   184   166   184   168   270   271   271   270   270   267   267   267   168   168   167         281   282   282   282
28    185   185   210   183   206   180   204   178   177   178   188   180   181   160   173   162   272   271   272   271   270   271   271   270   285   286   280   282         168   169   169
29    185   185   210   183   180   180   177   178   176   179   186   180   169   160   183   161   271   269   266   267   267   266   266   267   281   281   280   281   167         168   169
30    185   185   209   183   180   179   198   177   176   178   191   179   167   160   171   162   272   272   271   271   272   271   270   271   285   284   285   284   169   170         169
31    185   185   184   184   204   180   204   178   177   179   184   180   160   160   164   162   272   273   272   271   270   270   270   270   285   285   285   285   169   169   169
```

Running Rust:

```text
$ ./target/release/core-to-core-latency -b2
CPU: 13th Gen Intel(R) Core(TM) i9-13980HX
Num cores: 32
Num iterations per samples: 1000
Num samples: 300

2) Single-writer single-reader latency on two shared cache lines

           0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22    23    24    25    26    27    28    29    30    31
      0
      1   37
      2  142   156
      3  155   156    36
      4  154   141   172   140
      5  157   151   173   152    36
      6  141   140   171   140   172   140
      7  153   150   174   143   172   141    36
      8  140   140   172   140   172   140   148   140
      9  156   155   162   146   183   141   184   152    36
     10  134   133   138   133   162   134   163   137   164   147
     11  149   150   167   133   150   147   174   137   169   146    36
     12  136   137   136   136   166   139   167   140   167   152   144   144
     13  153   154   150   149   174   151   158   154   179   170   122   129    36
     14  138   138   139   137   168   140   168   141   169   155   145   146   172   155
     15  156   138   138   137   176   153   177   142   168   155   132   132   140   137    36
     16  153   153   153   154   189   159   189   164   180   173   166   164   183   190   185   193
     17  154   153   188   153   188   158   189   164   189   171   166   164   183   190   185   193   156
     18  154   153   188   153   188   158   189   164   189   171   166   165   183   190   185   193   157   159
     19  153   153   188   153   188   158   189   163   189   172   166   164   183   190   185   193   161   159   159
     20  154   153   156   153   187   159   188   164   188   172   165   164   183   190   186   192   270   268   266   273
     21  153   153   187   152   188   159   188   163   189   172   164   164   183   190   186   192   273   272   271   270   159
     22  153   153   188   152   187   159   188   164   172   172   165   164   183   190   186   193   273   272   269   274   158   158
     23  153   154   187   153   187   160   188   164   189   171   165   164   183   190   186   193   272   272   274   272   159   159   156
     24  161   160   197   160   197   164   198   166   198   181   175   174   192   200   194   202   287   288   287   284   286   286   286   286
     25  161   160   197   161   191   163   198   166   197   182   175   174   193   201   195   202   238   285   283   284   285   285   286   283   178
     26  161   161   197   161   197   164   198   166   198   182   175   174   201   201   195   202   289   287   288   287   288   289   288   286   179   180
     27  161   161   197   161   197   164   198   165   198   182   175   174   193   201   195   202   283   284   283   283   283   282   282   283   178   178   177
     28  161   161   194   161   173   164   199   166   199   182   176   174   193   201   195   202   287   288   286   287   285   286   285   287   301   300   302   302
     29  161   161   197   161   173   163   199   166   199   182   176   174   193   201   194   202   289   287   285   286   285   283   286   284   301   300   300   301   179
     30  161   161   198   161   184   163   199   166   199   181   176   174   193   201   202   202   287   288   287   287   287   287   287   286   302   301   302   301   180   181
     31  161   161   198   161   198   164   199   166   199   181   176   174   193   201   195   202   287   286   287   285   283   286   284   286   300   300   298   298   179   180   180

    Min  latency: 35.9ns ±0.0 cores: (15,14)
    Max  latency: 302.3ns ±0.8 cores: (28,27)
    Mean latency: 189.9ns
```

### 3.2. Intel i3-2120T

#### 3.2.1. Single Line CAS

```text
Running CAS Core Benchmark
 Samples: 500
 Iterations: 4000

      0     1     2     3
0           39    13    38
1     38          39    13
2     12    38          39
3     38    12    38
```

#### 3.2.2. Two Line Read/Write Ping/Pong

```text
Running Read/Write Core Benchmark
 Samples: 500
 Iterations: 4000

      0     1     2     3
0           51    19    51
1     51          51    19
2     20    51          51
3     51    19    52
```
