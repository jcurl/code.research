#ifndef CPUID_CPUID_H
#define CPUID_CPUID_H

#include <cstdint>

namespace rjcp::cpuid {

using cpuidreg = std::uint32_t;

/// @brief A CPUID Request.
struct cpuid_req {
  cpuidreg eax;  //< The leaf to query.
  cpuidreg ecx;  //< The subleaf to query.
};

/// @brief A CPUID response.
struct cpuid_res {
  cpuidreg eax;  //< The EAX register result.
  cpuidreg ebx;  //< The EBX register result.
  cpuidreg ecx;  //< The ECX register result.
  cpuidreg edx;  //< The EDX register result.
};

/// @brief A full CPUID request and response.
struct cpuid_info {
  struct cpuid_req req;  //< The CPUID request.
  struct cpuid_res res;  //< The CPUID response.
};

}  // namespace rjcp::cpuid

#endif