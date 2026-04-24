#include "config.h"

#include "cpuid/cpuid_cpuctl.h"

namespace rjcp::cpuid {

#if !HAVE_CPUCTL_CPUID_COUNT
[[nodiscard]] auto cpuid_cpuctl::has_cpuid() const -> bool { return false; }

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto cpuid_cpuctl::cpuid([[maybe_unused]] cpuidreg eax,
    [[maybe_unused]] cpuidreg ecx) -> std::optional<cpuid_res> {
  return std::nullopt;
}
#endif

}  // namespace rjcp::cpuid
