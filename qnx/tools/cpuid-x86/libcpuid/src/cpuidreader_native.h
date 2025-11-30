#ifndef CPUID_CPUIDREADER_NATIVE_H
#define CPUID_CPUIDREADER_NATIVE_H

#include <optional>
#include <thread>

#include "cpuid/cpuid.h"
#include "ubench/thread.h"
#include "cpuid_native.h"

namespace rjcp::cpuid {

/// @brief Query the CPUID using the native CPUID instruction.
class cpuidreader_native {
 public:
  cpuidreader_native() = default;
  cpuidreader_native(const cpuidreader_native&) = delete;
  cpuidreader_native(cpuidreader_native&&) = default;
  auto operator=(const cpuidreader_native&) -> cpuidreader_native& = delete;
  auto operator=(cpuidreader_native&&) -> cpuidreader_native& = default;
  ~cpuidreader_native() = default;

  /// @brief Checks if this class can query the CPUID.
  ///
  /// @return true if querying the CPUID should work, false otherwise.
  auto has_cpuid() -> bool { return true; }

  /// @brief Indicates if this instance queries current hardware.
  ///
  /// @return true if queries are on current hardware, false if queries are
  /// cached and may not reflect this machine.
  auto is_online() -> bool { return true; }

  /// @brief Get the number of cores available.
  ///
  /// @return the number of cores available.
  [[nodiscard]] auto cores() const -> unsigned int {
    return std::thread::hardware_concurrency();
  }

  /// @brief Query the CPUID for the register eax and ecx.
  ///
  /// @param eax The leaf to query for.
  ///
  /// @param ecx The subleaf to query for.
  ///
  /// @return the result of the query, or std::nullopt if the query failed.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  auto cpuid(cpuidreg eax, cpuidreg ecx) -> std::optional<cpuid_res> {
    return cpuid_.cpuid(eax, ecx);
  }

  /// @brief Enable CPUID instructions on the specified core, until object is
  /// destroyed.
  ///
  /// Ensure that the object returned is destructed on the same thread where it
  /// is created.
  ///
  /// @param core The core to pin to.
  ///
  /// @return An optional object, if it has the value, represents the state of
  /// the CPU when querying the CPUID.
  auto enable_core(unsigned int core)
      -> std::optional<ubench::thread::pin_core> {
    auto pinned = ubench::thread::pin_core(core);
    if (pinned) return pinned;
    return std::nullopt;
  }

 private:
  cpuid_native cpuid_{};
};

}  // namespace rjcp::cpuid

#endif
