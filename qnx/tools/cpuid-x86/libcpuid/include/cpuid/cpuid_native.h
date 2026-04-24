#ifndef CPUID_CPUID_NATIVE_H
#define CPUID_CPUID_NATIVE_H

#include <optional>

#include "cpuid/cpuid.h"

namespace rjcp::cpuid {

/// @brief Query the CPUID using the native CPUID instruction.
///
/// The class can be moved, but not copied.
class cpuid_native {
 public:
  cpuid_native() = default;
  cpuid_native(const cpuid_native&) = delete;
  cpuid_native(cpuid_native&&) = default;
  auto operator=(const cpuid_native&) -> cpuid_native& = delete;
  auto operator=(cpuid_native&&) -> cpuid_native& = default;
  ~cpuid_native() = default;

  /// @brief Checks if this class can query the CPUID.
  ///
  /// @return true if querying the CPUID should work, false otherwise.
  [[nodiscard]] auto has_cpuid() const -> bool;

  /// @brief Query the CPUID for the register eax and ecx.
  ///
  /// @param eax The leaf to query for.
  ///
  /// @param ecx The subleaf to query for.
  ///
  /// @return the result of the query, or std::nullopt if the query failed.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  auto cpuid(cpuidreg eax, cpuidreg ecx) -> std::optional<cpuid_res>;
};

}  // namespace rjcp::cpuid

#endif
