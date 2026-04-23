#include "config.h"

#include "cpuid/cpuid_native.h"

#if HAVE_CPUIDX86
#include <cpuid.h>
#endif

namespace rjcp::cpuid {

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto cpuid_native::cpuid([[maybe_unused]] cpuidreg eax,
    [[maybe_unused]] cpuidreg ecx) -> std::optional<cpuid_res> {
#if HAVE_CPUIDX86
  struct cpuid_res res {};

  __cpuid_count(eax, ecx, res.eax, res.ebx, res.ecx, res.edx);
  return res;
#else
  return std::nullopt;
#endif
}

[[nodiscard]] auto cpuid_native::has_cpuid() const -> bool {
  return HAVE_CPUIDX86 != 0;
}

}  // namespace rjcp::cpuid
