#ifndef CPUID_DUMP_INTEL_H
#define CPUID_DUMP_INTEL_H

#include "cpuid/cpuid.h"

namespace rjcp::cpuid {

/// @brief Dump all the leaves relevant for Intel.
///
/// @param reader The CPUID reader.
///
/// @return A vector of all registers dumped, for normal, extended and
/// Hypervisor.
auto dump_cpu_intel(cpuidreader& reader)
    -> std::optional<std::vector<cpuid_info>>;

}  // namespace rjcp::cpuid

#endif
