#include "cpuid/cpuid_native.h"

#include <cpuid.h>

namespace rjcp::cpuid {

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto cpuid_native::cpuid(cpuidreg eax, cpuidreg ecx)
    -> std::optional<cpuid_res> {
  struct cpuid_res res {};

  __cpuid_count(eax, ecx, res.eax, res.ebx, res.ecx, res.edx);
  return res;
}

}  // namespace rjcp::cpuid
