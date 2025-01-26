#include "config.h"

#if defined(__aarch64__) && HAVE_CXX_ARM64_LSE
#if defined(__QNX__)
#include <sys/syspage.h>
#endif

#if defined(__linux__)
#include <sys/auxv.h>
#include <asm/hwcap.h>
#endif
#endif

#include "arm64.h"

auto has_arm64_lse() -> bool {
#if !defined(__aarch64__) || !HAVE_CXX_ARM64_LSE
  // We weren't compiled with LSE extensions, so don't test.
  return false;
#elif defined(__QNX__)
  // Need to read the system pages to determine if the feature is supported on
  // the processor or not.
  return (SYSPAGE_ENTRY(cpuinfo)->flags & AARCH64_CPU_FLAG_LSE) != 0;
#elif defined(__linux__)
  unsigned long hwcaps = getauxval(AT_HWCAP);
  return (hwcaps & HWCAP_ATOMICS) != 0;
#else
  return false;
#endif
}
