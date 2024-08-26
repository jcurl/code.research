# Core Latency

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

## Design

### CAS Latency

The CAS (Compare and Swap) latency is a ping-pong system where one core waits to
switch from the value ping to pong, and another core wats to swith from pong to
ping. This measures the delays in the synchronisation of the caches between the
two cores.

#### Disassembly in C++ for x86_64

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

```txt
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

```txt
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

#### Optimisations for x86_64

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

## Results

### Intel i9-13980HX with P and E cores

The core-to-core for a single line (CAS) latencies. Turbo boost is disabled under Linux by:

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
