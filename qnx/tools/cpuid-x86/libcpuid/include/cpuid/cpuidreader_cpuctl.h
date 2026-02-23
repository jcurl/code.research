#ifndef CPUID_CPUIDREADER_CPUCTL_H
#define CPUID_CPUIDREADER_CPUCTL_H

#include <memory>
#include <optional>
#include <utility>

#include "cpuid/cpuid.h"
#include "cpuid/cpuid_cpuctl.h"
#include "ubench/thread.h"

namespace rjcp::cpuid {

namespace detail {

class cpuid_cpuctl_file;

}  // namespace detail

/// @brief Query the CPUID using the device /dev/cpuctlN
class cpuidreader_cpuctl : public cpuidreader {
 public:
  cpuidreader_cpuctl() = default;

  cpuidreader_cpuctl(const cpuidreader_cpuctl&) = delete;

  cpuidreader_cpuctl(cpuidreader_cpuctl&& other) noexcept
      : ctx_{std::exchange(other.ctx_, nullptr)} {}

  auto operator=(const cpuidreader_cpuctl&) -> cpuidreader_cpuctl& = delete;

  auto operator=(cpuidreader_cpuctl&& other) noexcept -> cpuidreader_cpuctl& {
    cpuidreader_cpuctl{std::move(other)}.swap(*this);
    return *this;
  }

  ~cpuidreader_cpuctl() = default;

  /// @brief Checks if this class can query the CPUID.
  ///
  /// @return true if querying the CPUID should work, false otherwise.
  auto has_cpuid() -> bool override;

  /// @brief Indicates if this instance queries current hardware.
  ///
  /// @return true if queries are on current hardware, false if queries are
  /// cached and may not reflect this machine.
  auto is_online() -> bool override { return true; }

  /// @brief Get the number of cores available.
  ///
  /// @return the number of cores available.
  [[nodiscard]] auto cores() const -> unsigned int override {
    return ubench::thread::thread_count();
  }

  /// @brief Query the CPUID for the register eax and ecx.
  ///
  /// @param eax The leaf to query for.
  ///
  /// @param ecx The subleaf to query for.
  ///
  /// @return the result of the query, or std::nullopt if the query failed.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  auto cpuid(cpuidreg eax, cpuidreg ecx) -> std::optional<cpuid_res> override;

  /// @brief Enable CPUID instructions on the specified core, until object is
  /// destroyed.
  ///
  /// Ensure that there is only one active thread at a time. If there is a core
  /// already active when this method is called, the second call will fail.
  ///
  /// @param core The core to pin to.
  ///
  /// @return An optional object, if it has the value, represents the state of
  /// the CPU when querying the CPUID.
  auto enable_core(unsigned int core)
      -> stdext::expected<std::unique_ptr<cpuid_ctx>, int> override;

 private:
  std::shared_ptr<detail::cpuid_cpuctl_file> ctx_{};

  auto swap(cpuidreader_cpuctl& other) noexcept -> void {
    std::swap(ctx_, other.ctx_);
  }
};

}  // namespace rjcp::cpuid

#endif
