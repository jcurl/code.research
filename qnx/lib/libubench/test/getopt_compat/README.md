# Options Class <!-- omit in toc -->

When prototyping `getopt()` the following issues were observed:

- Linux glibc `getopt()` uses a non-standard way to reset the state. It requires
  setting `optind = 0` when other implementations require `optind = 1`.
- Alpine (libmusl) doesn't clear the `optarg` if the argument had no value.
- Linux glibc `optopt` isn't reset when parsing a new argument vector, thus
  making it impossible to check properly the option `?`. e.g. if we parsed a
  vector and it found an unknown option, then `optopt` is correctly set to that
  option and `getopt()` returns `?`. If we now reset the vector, and expect a
  `?` (used often for help), then `optopt` will be still set to the previous
  (invalid) arg, and we'd think the user gave a wrong option, when they didn't.
  - This is because the glibc implementation manages a static structure and
    keeps a copy, so we can't even set `optopt` to `0` before the call, it will
    always be overwritten with the cached copy it has.
- Linux glibc provides the `optstring` with `::`, but it only works as an
  optional argument if there is no space. This sequence isn't support on some
  other operating systems.
- All implementations have global scope. This makes it impossible to parse
  option arguments in a thread-safe way.

Based on the difference in behaviour between the operating systems tested, the
decision is to create a new implementation in C++17 that doesn't require the
Posix `getopt()` implementation. Only short options will be considered here.

## 1. Behavioural Compatibility

In designing the class, we should base it off common behaviour between various
Operating Systems. See the test binary `ubench_getopt_test`, run as:

```sh
ubench_getopt_test --gtest_filter=ubench_options_dump.*
```

To read the table, you'll see a list of iterations when calling `getopt`. They
are represented as a tuple of `(getopt() result, optarg, optopt)`.

| Test Case             | optstring | args              | Ubuntu 22.04 (glibc-2.35) | Alpine (musl-1.2.5) | QNX 7.1         | QNX 8.0         | FreeBSD 14.2    | NetBSD 10.1     |
| --------------------- | --------- | ----------------- | ------------------------- | ------------------- | --------------- | --------------- | --------------- | --------------- |
| `noopts`              |           | `-f`              | (?,null,f)                | (?,null,f)          | (?,null,f)      | (?,null,f)      | (?,null,f)      | (?,null,f)      |
| `oneopt`              | `f`       | `-f`              | (f,null,f¹)               | (f,null,0)          | (f,null,f¹)     | (f,null,f¹)     | (f,null,f¹)     | (f,null,f¹)     |
| `oneopt_arg1`         | `f`       | `-f` `arg`        | (f,null,f¹)               | (f,null,0)          | (f,null,f¹)     | (f,null,f¹)     | (f,null,f¹)     | (f,null,f¹)     |
| `oneopt_arg2`         | `f`       | `-farg`           | (f,null,f¹)               | (f,null,0)          | (f,null,f¹)     | (f,null,f¹)     | (f,null,f¹)     | (f,null,f¹)     |
|                       |           |                   | (?,null,a)                | (?,null,a)          | (?,null,a)      | (?,null,a)      | (?,null,a)      | (?,null,a)      |
|                       |           |                   | (?,null,r)                | (?,null,r)          | (?,null,r)      | (?,null,r)      | (?,null,r)      | (?,null,r)      |
|                       |           |                   | (?,null,g)                | (?,null,g)          | (?,null,g)      | (?,null,g)      | (?,null,g)      | (?,null,g)      |
| `oneoptarg_none`      | `f:`      | `-f`              | (?,null,f)                | (?,und⁴,)           | (?,und⁴,)       | (?,und⁴,)       | (?,null,f)      | (?,null,f)      |
| `oneoptarg_arg`       | `f:`      | `-farg`           | (f,`arg`,f¹)              | (f,`arg`,0)         | (f,`arg`,f¹)    | (f,`arg`,f¹)    | (f,`arg`,f¹)    | (f,`arg`,f¹)    |
| `oneoptarg_arg2`      | `f:`      | `-f` `arg`        | (f,`arg`,f¹)              | (f,`arg`,0)         | (f,`arg`,f¹)    | (f,`arg`,f¹)    | (f,`arg`,f¹)    | (f,`arg`,f¹)    |
| `oneoptarg_equals`    | `f:`      | `-f=arg`          | (f,`=arg`,f¹)             | (f,`=arg`,0)        | (f,`=arg`,f¹)   | (f,`=arg`,f¹)   | (f,`=arg`,f¹)   | (f,`=arg`,f¹)   |
| `oneoptarg_alt_none`  | `:f:`     | `-f`              | (:,null,f)                | (:,und⁴,)           | (:,und⁴,)       | (:,und⁴,)       | (:,null,f)      | (:,null,f)      |
| `oneoptarg_alt_arg`   | `:f:`     | `-farg`           | (f,`arg`,f¹)              | (f,`arg`,0)         | (f,`arg`,f¹)    | (f,`arg`,f¹)    | (f,`arg`,f¹)    | (f,`arg`,f¹)    |
| `oneoptarg_alt_arg2`  | `:f:`     | `-f` `arg`        | (f,`arg`,f¹)              | (f,`arg`,0)         | (f,`arg`,f¹)    | (f,`arg`,f¹)    | (f,`arg`,f¹)    | (f,`arg`,f¹)    |
| `twooptsmix`          | `:f:a`    | `-farg`           | (f,`arg`,f¹)              | (f,`arg`,0)         | (f,`arg`,f¹)    | (f,`arg`,f¹)    | (f,`arg`,f¹)    | (f,`arg`,f¹)    |
| `twooptsmix2`         | `:f:a`    | `-afarg`          | (a,null,f¹)               | (a,null,0)          | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     |
|                       |           |                   | (f,`arg`,f¹)              | (f,`arg`,0)         | (f,`arg`,f¹)    | (f,`arg`,f¹)    | (f,`arg`,f¹)    | (f,`arg`,f¹)    |
| `topoptsmix3`         | `:f:a`    | `-a` `-farg`      | (a,null,f¹)               | (a,null,0)          | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     |
|                       |           |                   | (f,`arg`,f¹)              | (f,`arg`,0)         | (f,`arg`,f¹)    | (f,`arg`,f¹)    | (f,`arg`,f¹)    | (f,`arg`,f¹)    |
| `twooptsmix4`         | `:f:a`    | `-af` `-farg`     | (a,null,f¹)               | (a,null,0)          | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     |
|                       |           |                   | (f,`-farg`,f¹)            | (f,`-farg`,0)       | (f,`-farg`,f¹)  | (f,`-farg`,f¹)  | (f,`-farg`,f¹)  | (f,`-farg`,f¹)  |
| `argfirst_noopts`     | `f`       | `arg`             | †                         | †                   | †               | †               | †               | †               |
| `argfirst_oneopt`     | `f`       | `arg` `-f`        | (f,null,f¹)               | †⁵                  | †⁵              | †⁵              | †⁵              | †⁵              |
| `argfirst2_oneopt`    | `f`       | `arg` `-f` `arg2` | (f,null,f¹) ²             | †⁵                  | †⁵              | †⁵              | †⁵              | †⁵              |
| `invalidopt`          | `f`       | `-z`              | (?,null,z)                | (?,null,z)          | (?,null,z)      | (?,null,z)      | (?,null,z)      | (?,null,z)      |
| `invalidopt_arg`      | `f`       | `-z` `arg`        | (?,null,z) ³              | (?,null,z) ³        | (?,null,z) ³    | (?,null,z) ³    | (?,null,z) ³    | (?,null,z) ³    |
| `invalidopt_char1`    | `f:`      | `-:` `arg`        | (?,null,:)                | (?,null,:)          | (?,null,:)      | (?,null,:)      | (?,null,:)      | (?,null,:)      |
| `invalidopt_char2`    | `f:<`     | `-<` `arg`        | (<,null,:¹)               | (<,null,0)          | (<,null,<)      | (<,null,<)      | (<,null,<)      | (<,null,<)      |
| `invalidopt_char3`    | `f:@`     | `-@` `arg`        | (@,null,:¹)               | (@,null,0)          | (@,null,@)      | (@,null,@)      | (@,null,@)      | (@,null,@)      |
| `invalidopt_char4`    | `f:;`     | `-;` `arg`        | (?,null,;) ⁹              | (;,null,0)          | (;,null,;)      | (;,null,;)      | (;,null,;)      | (;,null,;)      |
| `help`                | `abcf?`   | `-?`              | (?,null,z¹)               | (?,null,0)          | (?,null,?⁶)     | (?,null,?⁶)     | (?,null,?⁶)     | (?,null,?⁶)     |
| `missing_arg_twoargs` | `f:a`     | `-f` `-a`         | (f,`-a`,z¹)               | (f,`-a`,0)          | (f,`-a`,f¹)     | (f,`-a`,f¹)     | (f,`-a`,f¹)     | (f,`-a`,f¹)     |
| `argwhitespace`       | `f:a`     | `-f␣` `-a`        | (f,`␣`,z¹)                | (f,`␣`,0)           | (f,`␣`,f¹)      | (f,`␣`,f¹)      | (f,`␣`,f¹)      | (f,`␣`,f¹)      |
|                       |           |                   | (a,null,z¹)               | (a,null,0)          | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     |
| `minus1`              | `f:a`     | `-`               | †                         | †                   | †               | †               | †               | †               |
| `minus2`              | `f:a`     | `-a` `-`          | (a,null,z¹)               | (a,null,0)          | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     |
| `minus3`              | `f:a`     | `-` `-a`          | (a,null,z¹) ⁷             | ⁵                   | ⁵               | ⁵               | ⁵               | ⁵               |
| `minus4`              | `f:a`     | `-f` `-`          | (f,`-`,z¹)                | (f,`-`,0)           | (f,`-`,f¹)      | (f,`-`,f¹)      | (f,`-`,f¹)      | (f,`-`,f¹)      |
| `doubleminus1`        | `f:a`     | `--`              | †                         | †                   | †               | †               | †               | †               |
| `doubleminus2`        | `f:a`     | `-a` `--`         | (a,null,z¹)               | (a,null,0)          | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     |
| `doubleminus3`        | `f:a`     | `--` `-a`         | ⁸                         | ⁸                   | ⁸               | ⁸               | ⁸               | ⁸               |
| `doubleminus4`        | `f:a`     | `-f` `--`         | (f,`--`,z¹)               | (f,`--`,0)          | (f,`--`,f¹)     | (f,`--`,f¹)     | (f,`--`,f¹)     | (f,`--`,f¹)     |
| `longoption1`         | `f:a`     | `--long`          | (?,null,-)                | (?,null,-)          | (?,null,-)      | (?,null,-)      | (?,null,-)      | (?,null,-)      |
|                       |           |                   | (?,null,l)                | (?,null,l)          | (?,null,l)      | (?,null,l)      | (?,null,l)      | (?,null,l)      |
|                       |           |                   | (?,null,o)                | (?,null,o)          | (?,null,o)      | (?,null,o)      | (?,null,o)      | (?,null,o)      |
|                       |           |                   | (?,null,n)                | (?,null,n)          | (?,null,n)      | (?,null,n)      | (?,null,n)      | (?,null,n)      |
|                       |           |                   | (?,null,g)                | (?,null,g)          | (?,null,g)      | (?,null,g)      | (?,null,g)      | (?,null,g)      |
| `longoption2`         | `f:a`     | `-f` `--long`     | (f,`--long`,g¹)           | (f,`--long`,0)      | (f,`--long`,f¹) | (f,`--long`,f¹) | (f,`--long`,f¹) | (f,`--long`,f¹) |
| `longoption3`         | `f:a`     | `-a` `--long`     | (a,null,g¹)               | (a,null,0)          | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     |
|                       |           |                   | (?,null,-)                | (?,null,-)          | (?,null,-)      | (?,null,-)      | (?,null,-)      | (?,null,-)      |
|                       |           |                   | (?,null,l)                | (?,null,l)          | (?,null,l)      | (?,null,l)      | (?,null,l)      | (?,null,l)      |
|                       |           |                   | (?,null,o)                | (?,null,o)          | (?,null,o)      | (?,null,o)      | (?,null,o)      | (?,null,o)      |
|                       |           |                   | (?,null,n)                | (?,null,n)          | (?,null,n)      | (?,null,n)      | (?,null,n)      | (?,null,n)      |
|                       |           |                   | (?,null,g)                | (?,null,g)          | (?,null,g)      | (?,null,g)      | (?,null,g)      | (?,null,g)      |
| `longoption4`         | `f:a`     | `--long` `-a`     | (?,null,-)                | (?,null,-)          | (?,null,-)      | (?,null,-)      | (?,null,-)      | (?,null,-)      |
|                       |           |                   | (?,null,l)                | (?,null,l)          | (?,null,l)      | (?,null,l)      | (?,null,l)      | (?,null,l)      |
|                       |           |                   | (?,null,o)                | (?,null,o)          | (?,null,o)      | (?,null,o)      | (?,null,o)      | (?,null,o)      |
|                       |           |                   | (?,null,n)                | (?,null,n)          | (?,null,n)      | (?,null,n)      | (?,null,n)      | (?,null,n)      |
|                       |           |                   | (?,null,g)                | (?,null,g)          | (?,null,g)      | (?,null,g)      | (?,null,g)      | (?,null,g)      |
|                       |           |                   | (a,null,g¹)               | (a,null,g¹)         | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     |
| `vendoropt1`          | `f:aW`    | `-a` `-W` `X`     | (a,null,g¹)               | (a,null,0)          | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     |
|                       |           |                   | (W,null,g¹)               | (W,null,0)          | (W,null,W¹)     | (W,null,W¹)     | (W,null,W¹)     | (W,null,W¹)     |
| `vendoropt2`          | `f:aW`    | `-W` `-a` `X`     | (W,null,g¹)               | (W,null,0)          | (W,null,W¹)     | (W,null,W¹)     | (W,null,W¹)     | (W,null,W¹)     |
|                       |           |                   | (a,null,g¹)               | (a,null,0)          | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     |
| `vendoropt3`          | `f:aW:`   | `-a` `-W` `X`     | (a,null,g¹)               | (a,null,0)          | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     | (a,null,a¹)     |
|                       |           |                   | (W,`X`,g¹)                | (W,`X`,0)           | (W,`X`,W¹)      | (W,`X`,W¹)      | (W,`X`,W¹)      | (W,`X`,W¹)      |

Notes:

- † No options returned.
- ¹ Error that `optopt` isn't reset, it should be 0.
- ² The value of `optind` is incorrect, it skipped over `arg` and is `3`
  pointing to `arg2` for the permuted arguments `prog_name -f arg arg2`.
- ³ `optind` points to the first argument.
- ⁴ undefined behaviour (corrupted string). On QNX-8.0, musl-1.2.5 test the
  test case crashed (segmentation fault).
- ⁵ appears to stop after the first non-option argument.
- ⁶ appears that `optarg` of `?` can be used to know this is the correct
  argument given.
- ⁷ the `-` arg is treated like a normal argument. On glibc, it's permuted to
  the end.
- ⁸ parsing stops at the `--` and everything is treated as normal arguments.
- ⁹ Treated as a valid option by some implementations, invalid for others, when
  it is not `isalnum`.

See [OpenPUB Utility
Conventions](https://pubs.opengroup.org/onlinepubs/009696799/basedefs/xbd_chap12.html#tag_12_02),
which describes how options are parsed.

- The `-` when parsed stops parsing and the `optind` is not incremented. This
  allows the `-` to be seen as an argument (e.g. by convention it is often used
  to represent `stdin`). On glibc, per ⁷, it is permuted to the end, when the
  standard says to stop parsing (no permutation of arguments that aren't options
  are arguments to options).
- The `--` when parsed stops parsing and the `optind` is incremented. This skips
  over that argument that the application doesn't see it.
- Some implementations ignore the requirement for `isalnum` and return single
  character options.
