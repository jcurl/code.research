#include "cpuid/cpuid_dev.h"

#include <unistd.h>

#include <optional>

namespace rjcp::cpuid {

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto cpuid_dev::cpuid(cpuidreg eax, cpuidreg ecx) -> std::optional<cpuid_res> {
  if (!has_cpuid()) return std::nullopt;

  struct cpuid_res res {};

  off_t offset = eax | static_cast<off_t>(ecx) << 32;
  auto result = pread(fd_, &res, 16, offset);
  if (result <= 0) {
    // We have an error, so we don't read any more values.
    fd_ = -1;
    return std::nullopt;
  }

  return res;
}

}  // namespace rjcp::cpuid
