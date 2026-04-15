#ifndef CPUID_CPUID_CACHE_H
#define CPUID_CPUID_CACHE_H

#include <optional>
#include <unordered_map>
#include <vector>

#include "cpuid/cpuid.h"

namespace rjcp::cpuid {

/// @brief Query the CPUID using cached values.
///
/// The class can be moved, but not copied.
class cpuid_cache {
 public:
  cpuid_cache() = default;

  /// @brief Initialises and populates the initial cache values.
  ///
  /// @param info The set of values that should be initially populated for this
  /// instance.
  cpuid_cache(const std::vector<cpuid_info>& info);

  cpuid_cache(const cpuid_cache&) = delete;
  cpuid_cache(cpuid_cache&&) = default;
  auto operator=(const cpuid_cache&) -> cpuid_cache& = delete;
  auto operator=(cpuid_cache&&) -> cpuid_cache& = default;
  ~cpuid_cache() = default;

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

  /// @brief Updates the register value in the cache.
  ///
  /// The value is always replaced. The return value indicates whether the value
  /// existed before or not.
  ///
  /// @param info The information to update in the cache.
  ///
  /// @return true if the value is new, false otherwise indicating it existed
  /// and is now replaced.
  auto update_cache(const cpuid_info& info) -> bool;

 private:
  std::unordered_map<cpuid_req, cpuid_info> map_{};
};

}  // namespace rjcp::cpuid

#endif
