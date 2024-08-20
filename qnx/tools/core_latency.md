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

#### Disassembly in C++

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
    3358:       89 f8                   mov    %edi,%eax
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
0           21    101   101   99    99    101   100   99    99    97    97    95    96    97    97    100   99    100   100   97    97    97    97    95    95    95    95    86    86    86    86
1     21          100   101   99    99    101   101   99    100   97    97    95    95    97    97    100   100   100   100   97    97    97    97    95    95    95    95    86    86    86    86
2     101   101         21    96    97    98    98    97    97    95    95    93    93    94    94    99    99    99    99    96    96    96    96    95    95    95    95    86    86    86    86
3     101   101   21          97    97    98    98    97    97    95    94    93    93    94    94    99    99    99    99    97    96    96    96    95    95    95    95    86    86    86    86
4     99    99    97    97          21    97    97    96    95    93    93    92    92    93    93    98    98    98    98    95    95    95    95    93    93    93    93    85    85    85    85
5     99    99    97    97    20          97    97    96    96    93    93    92    92    93    93    98    98    98    98    95    95    95    95    93    94    93    93    85    85    85    85
6     100   101   98    98    97    97          21    96    97    94    94    93    93    94    94    97    97    97    97    95    95    95    95    93    93    93    93    84    84    84    84
7     101   101   98    98    97    97    21          97    97    94    94    93    93    94    94    97    97    97    97    95    95    95    95    93    93    93    93    84    84    84    84
8     99    99    97    97    95    95    97    97          21    93    93    92    92    93    93    96    96    96    96    93    93    93    93    92    92    92    92    82    82    82    82
9     99    99    97    97    95    96    97    97    20          93    93    92    92    93    93    96    96    96    96    93    93    93    93    92    91    92    92    82    82    82    82
10    97    97    94    95    93    93    94    94    93    93          21    89    90    91    91    95    95    95    95    93    93    93    93    91    91    91    91    82    82    82    82
11    97    97    94    94    93    93    94    94    93    93    21          89    89    91    91    95    95    95    95    93    93    93    93    91    91    91    91    82    82    82    82
12    95    96    93    93    92    92    93    93    92    92    89    89          21    89    89    94    94    94    94    92    91    91    91    90    90    90    90    81    81    81    81
13    96    96    93    93    92    92    93    93    92    92    90    90    21          89    89    94    94    94    94    92    92    91    92    90    90    90    90    81    81    81    81
14    97    97    94    94    93    93    94    94    93    93    91    91    89    89          21    93    93    93    93    91    91    91    91    89    89    89    89    80    80    80    80
15    97    97    94    94    93    93    94    94    93    93    91    90    89    89    21          93    93    93    93    91    91    91    91    89    89    89    89    80    80    80    80
16    99    99    98    98    97    97    96    96    95    95    95    95    93    93    93    93          152   152   152   129   129   130   130   127   125   126   127   106   106   106   106
17    99    99    98    98    97    97    96    96    95    95    95    95    93    93    93    93    152         151   152   129   129   129   130   127   127   127   125   106   106   106   106
18    99    99    98    98    97    97    96    96    95    95    95    95    93    93    93    93    152   152         152   127   129   129   130   126   127   127   127   106   106   105   106
19    99    99    98    98    97    97    96    96    95    95    95    95    93    93    93    93    152   152   152         130   129   128   130   127   127   127   127   106   106   106   106
20    97    97    96    96    94    94    94    94    93    93    92    92    91    91    91    91    132   131   132   131         147   147   147   126   125   125   125   104   104   104   105
21    97    97    96    96    94    95    94    94    93    93    92    92    91    91    91    91    132   129   130   119   147         147   147   126   126   125   124   105   104   104   104
22    97    97    96    96    95    94    94    94    93    93    92    92    91    91    91    91    131   132   131   131   146   147         147   125   125   125   123   104   104   104   103
23    97    97    96    96    94    95    94    94    93    93    92    92    91    91    91    91    132   131   131   131   147   147   146         125   124   125   126   105   104   104   105
24    95    95    94    94    93    93    92    92    91    91    90    90    89    89    89    89    127   128   128   128   126   125   126   125         144   143   144   97    98    98    98
25    95    95    94    94    93    93    92    92    91    91    90    90    89    89    89    89    128   128   128   127   120   126   125   125   144         144   140   100   101   100   100
26    95    95    94    94    93    93    92    92    91    91    90    90    89    89    89    89    129   129   129   128   125   126   125   125   145   145         145   101   101   101   101
27    95    95    94    94    93    93    92    92    91    91    90    90    89    89    89    89    126   128   128   128   126   125   126   126   145   144   145         101   101   101   101
28    97    97    95    96    95    95    94    94    93    93    92    91    90    90    90    90    110   110   110   110   110   110   110   110   107   107   107   107         138   138   137
29    97    97    97    95    95    95    95    95    93    93    91    91    90    90    90    90    110   110   110   110   110   110   110   110   106   107   107   107   138         138   138
30    97    97    96    96    95    95    94    94    93    93    92    91    89    89    90    90    110   110   110   110   110   110   110   110   107   107   107   107   138   137         138
31    97    97    96    96    94    95    95    95    93    93    91    91    90    90    90    90    110   110   110   110   110   110   109   110   107   107   107   107   138   138   138
```

In these results we can clearly see:

- Cores 0-15 are the P-Cores and there is hyperthreading enabled
  - Hyperthreading shows approximately 20ns between cores (0-1, 2-3, 4-5. 6-7.
    8-9. 10-11, 12-13. 14-15).
- P-core to P-core is normally 80-100ns latency
- E-core to E-core is 110-150ns latency

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
