#include "cpuid/cpuid_cpuctl.h"

#include <sys/types.h>
#include <sys/cpuctl.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace rjcp::cpuid {

[[nodiscard]] auto cpuid_cpuctl::has_cpuid() const -> bool { return fd_ >= 0; }

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto cpuid_cpuctl::cpuid(cpuidreg eax, cpuidreg ecx)
    -> std::optional<cpuid_res> {
  if (!has_cpuid()) return std::nullopt;

  struct cpuid_res res {};

  cpuctl_cpuid_count_args_t args;
  args.level = static_cast<int>(eax);
  args.level_type = static_cast<int>(ecx);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  int idresult = ioctl(fd_, CPUCTL_CPUID_COUNT, &args);
  if (idresult == -1) {
    // We have an error, but we can't say what the error is.
    return std::nullopt;
  }

  res.eax = args.data[0];
  res.ebx = args.data[1];
  res.ecx = args.data[2];
  res.edx = args.data[3];
  return res;
}

}  // namespace rjcp::cpuid
