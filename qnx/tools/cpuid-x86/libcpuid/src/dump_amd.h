#ifndef CPUID_DUMP_AMD_H
#define CPUID_DUMP_AMD_H

#include "cpuid/cpuid.h"

namespace rjcp::cpuid {

/// @brief Dump all the leaves relevant for AMD.
///
/// @param reader The CPUID reader.
///
/// @return A vector of all registers dumped, for normal, extended and
/// Hypervisor.
auto dump_cpu_amd(cpuidreader& reader)
    -> std::optional<std::vector<cpuid_info>>;

}  // namespace rjcp::cpuid

#endif
