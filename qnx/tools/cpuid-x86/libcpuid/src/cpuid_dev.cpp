#include "config.h"

#if HAVE_FILE_OFFSET_BITS_64
// Needed to enable 64-bit off_t for 32-bit compilations
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage,bugprone-reserved-identifier)
#define _FILE_OFFSET_BITS 64
#endif

#include <unistd.h>

#include "cpuid/cpuid_dev.h"

namespace rjcp::cpuid {

[[nodiscard]] auto cpuid_dev::has_cpuid() const -> bool {
  return HAVE_OFF_T_64BIT && fd_ >= 0;
}

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
