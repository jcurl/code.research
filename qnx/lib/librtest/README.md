# Library rtest <!-- omit from toc -->

Google Test helpers. Include `rtest/rtest.h`.

- [1. Instructions for Choosing a Macro](#1-instructions-for-choosing-a-macro)
- [2. Rules](#2-rules)
- [3. Examples](#3-examples)
  - [3.1. Checking Optional](#31-checking-optional)
  - [3.2. EXPECT\_OPTIONAL\_XXX](#32-expect_optional_xxx)
  - [3.3. ASSERT\_OPTIONAL\_XXX](#33-assert_optional_xxx)
  - [3.4. Optional with Errors](#34-optional-with-errors)
  - [3.5. Checks in a Func() -\> bool](#35-checks-in-a-func---bool)


## 1. Instructions for Choosing a Macro

Let `x` be a `std::optional` or `stdext::expected` value.

1. Value must exist, and must be true/false:
   - `ASSERT_OPTIONAL_TRUE(x)` / `ASSERT_OPTIONAL_FALSE(x)`; fatal on failure.
   - `EXPECT_OPTIONAL_TRUE(x)` / `EXPECT_OPTIONAL_FALSE(x)`; a missing value is
     fatal, a wrong value is non-fatal.
2. Value must exist, and must equal/not equal `v`:
   - `ASSERT_OPTIONAL_EQ(x, v)` / `ASSERT_OPTIONAL_NE(x, v)`.
   - `EXPECT_OPTIONAL_EQ(x, v)` / `EXPECT_OPTIONAL_NE(x, v)`.
3. Only check that a value exists:
   - `ASSERT_OPTIONAL(x)`; fatal if there is no value.
4. Value must be missing:
   - `ASSERT_OPTIONAL_HAS_ERROR(x)`; fatal if there is a value. Works with any
     optional type.
   - With `x.error()` equal to `e` (`stdext::expected` only):
     - `ASSERT_OPTIONAL_ERROR(x, e)`; fatal on failure.
     - `EXPECT_OPTIONAL_ERROR(x, e)`; a wrong error is non-fatal.
5. The call may succeed or fail, and only a failure is checked:
   - `MAYBE_OPTIONAL_ERROR(x, e)`. If `x` has a value, nothing is checked or
     printed. If `x` has no value, it behaves like `EXPECT_OPTIONAL_ERROR`.
6. Inside a function returning `bool`, stop on failure:
   - `EXPECT_EQ_OR_RETURN(expected, actual)`
   - `EXPECT_NE_OR_RETURN(expected, actual)`
   - `EXPECT_TRUE_OR_RETURN(condition)`
   - `EXPECT_FALSE_OR_RETURN(condition)`
   - Arguments may be evaluated more than once.

## 2. Rules

- Use `ASSERT_*` when later code depends on the value. Use `EXPECT_*` otherwise.
- For `EXPECT_*` on an optional that may be missing, call `ASSERT_OPTIONAL(x)`
  first. Otherwise a missing value is fatal.
- Stream a message after a macro to add context. It is printed only when the
  value is missing: `ASSERT_OPTIONAL_TRUE(r) << perror(r.error());`
- Do not use `*_ERROR` macros with `std::optional`; it has no `error()`.

## 3. Examples

The examples show the typical usage of the new macros, and how it was done with
the GTest macros only. Note, that even though code execution stops with an
ASSERT, tools such as clang-tidy (currently up to 22.1.3) are not able to detect
properly that the function ends, and will report errors on optional access.

### 3.1. Checking Optional

Value must exist (`ASSERT_OPTIONAL`):

```cpp
auto ores = udp.open(localsock);
// with gtest macros
ASSERT_TRUE(ores.has_value()) << ubench::string::perror(ores.error());
// with rtest macros
ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());
```

### 3.2. EXPECT_OPTIONAL_XXX

Value must exist and should be true (`EXPECT_OPTIONAL_TRUE`):

```cpp
auto nmlb = udp.get_multicast_loopback();
// with gtest macros
ASSERT_TRUE(nmlb.has_value()) << ubench::string::perror(nmlb.error());
if (nmlb) {
  EXPECT_TRUE(*nmlb);
}
// with rtest macros
EXPECT_OPTIONAL_TRUE(nmlb) << ubench::string::perror(nmlb.error());
```

Value must exist and should be false (`EXPECT_OPTIONAL_FALSE`):

```cpp
udp.set_multicast_loopback(false);
auto nmlb = udp.get_multicast_loopback();
// with gtest macros
ASSERT_TRUE(nmlb.has_value()) << ubench::string::perror(nmlb.error());
if (nmlb) {
  EXPECT_FALSE(*nmlb);
}
// with rtest macros
EXPECT_OPTIONAL_FALSE(nmlb) << ubench::string::perror(nmlb.error());
```

Value must exist and should equal 5 (`EXPECT_OPTIONAL_EQ`):

```cpp
auto nmttl = udp.get_multicast_ttl();
// with gtest macros
ASSERT_TRUE(nmttl.has_value()) << ubench::string::perror(nmttl.error());
if (nmttl) {
  EXPECT_EQ(*nmttl, 5);
}
// with rtest macros
EXPECT_OPTIONAL_EQ(nmttl, 5) << ubench::string::perror(nmttl.error());
```

Value must exist and should not equal 0 (`EXPECT_OPTIONAL_NE`):

```cpp
auto sres = udp.send(destsock, packet);
// with gtest macros
ASSERT_TRUE(sres.has_value()) << ubench::string::perror(sres.error());
if (sres) {
  EXPECT_NE(*sres, 0);
}
// with rtest macros
EXPECT_OPTIONAL_NE(sres, 0) << ubench::string::perror(sres.error());
```

Value must be missing with error `EINVAL` (`EXPECT_OPTIONAL_ERROR`):

```cpp
auto sendbuf = udp.get_sendbuf();
// with gtest macros
ASSERT_FALSE(sendbuf.has_value());
EXPECT_EQ(sendbuf.error(), EINVAL);
// with rtest macros
EXPECT_OPTIONAL_ERROR(sendbuf, EINVAL);
```

### 3.3. ASSERT_OPTIONAL_XXX

Value must exist and must be true, fatal on failure (`ASSERT_OPTIONAL_TRUE`):

```cpp
auto nmlb = udp.get_multicast_loopback();
// with gtest macros
ASSERT_TRUE(nmlb.has_value()) << ubench::string::perror(nmlb.error());
if (nmlb) {
  ASSERT_TRUE(*nmlb);
}
// with rtest macros
ASSERT_OPTIONAL_TRUE(nmlb) << ubench::string::perror(nmlb.error());
```

Value must exist and must be false, fatal on failure (`ASSERT_OPTIONAL_FALSE`):

```cpp
auto nmlb = udp.get_multicast_loopback();
// with gtest macros
ASSERT_TRUE(nmlb.has_value()) << ubench::string::perror(nmlb.error());
if (nmlb) {
  ASSERT_FALSE(*nmlb);
}
// with rtest macros
ASSERT_OPTIONAL_FALSE(nmlb) << ubench::string::perror(nmlb.error());
```

Value must exist and must equal 4, fatal on failure (`ASSERT_OPTIONAL_EQ`):

```cpp
auto sres = udp.send(destsock, packet);
// with gtest macros
ASSERT_TRUE(sres.has_value()) << ubench::string::perror(sres.error());
if (sres) {
  ASSERT_EQ(*sres, 4);
}
// with rtest macros
ASSERT_OPTIONAL_EQ(sres, 4) << ubench::string::perror(sres.error());
```

Value must exist and must not equal 0, fatal on failure (`ASSERT_OPTIONAL_NE`):

```cpp
auto nmttl = udp.get_multicast_ttl();
// with gtest macros
ASSERT_TRUE(nmttl.has_value()) << ubench::string::perror(nmttl.error());
if (nmttl) {
  ASSERT_NE(*nmttl, 0);
}
// with rtest macros
ASSERT_OPTIONAL_NE(nmttl, 0) << ubench::string::perror(nmttl.error());
```

### 3.4. Optional with Errors

Value must be missing, fatal on failure (`ASSERT_OPTIONAL_HAS_ERROR`, any optional):

```cpp
auto cpuid = cpu.cpuid(0x80000001, 0x00000000);
// with gtest macros
ASSERT_FALSE(cpuid.has_value());
// with rtest macros
ASSERT_OPTIONAL_HAS_ERROR(cpuid);
```

Value must be missing and must have the error `EINVAL`, fatal on failure (`ASSERT_OPTIONAL_ERROR`):

```cpp
auto sendbuf = udp.get_sendbuf();
// with gtest macros
ASSERT_FALSE(sendbuf.has_value());
ASSERT_EQ(sendbuf.error(), EINVAL);
// with rtest macros
ASSERT_OPTIONAL_ERROR(sendbuf, EINVAL);
```

If value doesn't exist, it may succeed, or fail with `ENOTSUP`
(`MAYBE_OPTIONAL_ERROR`):

```cpp
auto reuse = udp.get_reuse_addr();
// with gtest macros
EXPECT_TRUE(reuse.has_value() || reuse.error() == ENOTSUP);
// with rtest macros
MAYBE_OPTIONAL_ERROR(reuse, ENOTSUP);
```

### 3.5. Checks in a Func() -> bool

Inside a function returning `bool`, the following return `false` on failure.
The gtest version returns the check's result, so it must be the last statement.

Values should be equal, returns `false` on error (`EXPECT_EQ_OR_RETURN`):

```cpp
ubench::net::udp udp{};
// with gtest macros
EXPECT_EQ(-1, static_cast<int>(udp));
return static_cast<int>(udp) == -1;
// with rtest macros
EXPECT_EQ_OR_RETURN(-1, static_cast<int>(udp));
```

Values should not be equal, returns `false` on error (`EXPECT_NE_OR_RETURN`):

```cpp
auto ores = udp.open(localsock);
// with gtest macros
EXPECT_NE(-1, static_cast<int>(udp));
return static_cast<int>(udp) != -1;
// with rtest macros
EXPECT_NE_OR_RETURN(-1, static_cast<int>(udp));
```

Condition should be true, returns `false` on error (`EXPECT_TRUE_OR_RETURN`):

```cpp
auto ores = udp.open(localsock);
// with gtest macros
EXPECT_TRUE(ores.has_value());
return ores.has_value();
// with rtest macros
EXPECT_TRUE_OR_RETURN(ores.has_value());
```

Condition should be false, returns `false` on error (`EXPECT_FALSE_OR_RETURN`):

```cpp
ubench::net::udp udp{};
// with gtest macros
EXPECT_FALSE(static_cast<int>(udp) == -1);
return static_cast<int>(udp) != -1;
// with rtest macros
EXPECT_FALSE_OR_RETURN(static_cast<int>(udp) == -1);
```
