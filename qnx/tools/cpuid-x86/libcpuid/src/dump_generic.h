#ifndef CPUID_DUMP_GENERIC_H
#define CPUID_DUMP_GENERIC_H

#include "cpuid/cpuid.h"

namespace rjcp::cpuid {

/// @brief Get the CPUID information from the reader.
///
/// @param reader The CPUID reader.
///
/// @param eax The leaf to read
///
/// @param ecx The subleaf to read
///
/// @return The results of the read, or std::nullopt if unsuccessful.
auto get_cpuid(cpuidreader& reader, cpuidreg eax, cpuidreg ecx)
    -> std::optional<cpuid_info>;

/// @brief Copy from register B to the end, each leaf. No subleaves are queried.
///
/// @tparam B The register to start copying from.
///
/// @param reader The CPUID reader to use to obtain the values using the current
/// context.
///
/// @param base The starting register to dump from.
///
/// @param registers The vector to copy the values into. Values are copied at
/// the end, irrespective if the values are already present o rnot.
///
/// @return true if there was at least one value copied, false otherwise.
auto dump_range(cpuidreader& reader, cpuidreg base,
    std::vector<cpuid_info>& registers) -> bool;

/// @brief Look through the registers if the hypervisor bit is set, and if so,
/// add the hypervisor set to the registers.
///
/// @param reader The CPUID reader to use to obtain the values using the current
/// context.
///
/// @param registers The vector to copy the values into. Values are copied at
/// the end, irrespective if the values are already present o rnot.
///
/// @return true if there was at least one value copied, false otherwise. If the
/// hypervisor bit is not set, then true is still returned as there was no
/// error.
auto dump_hypervisor(cpuidreader& reader, std::vector<cpuid_info>& registers)
    -> bool;

/// @brief Dump all the leaves but no subleaves.
///
/// @param reader The CPUID reader.
///
/// @return A vector of all registers dumped, for normal, extended and
/// Hypervisor.
auto dump_cpu_generic(cpuidreader& reader)
    -> std::optional<std::vector<cpuid_info>>;

}  // namespace rjcp::cpuid

#endif
