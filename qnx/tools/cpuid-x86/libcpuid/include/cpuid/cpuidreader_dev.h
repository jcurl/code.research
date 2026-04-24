#ifndef CPUID_CPUIDREADER_DEV_H
#define CPUID_CPUIDREADER_DEV_H

#include <memory>
#include <optional>
#include <utility>

#include "cpuid/cpuid.h"
#include "cpuid/cpuid_dev.h"
#include "ubench/thread.h"

namespace rjcp::cpuid {

namespace detail {

class cpuid_dev_file;

}  // namespace detail

/// @brief Query the CPUID using the device /dev/cpu/N/cpuid
class cpuidreader_dev : public cpuidreader {
 public:
  cpuidreader_dev() = default;

  cpuidreader_dev(const cpuidreader_dev&) = delete;

  cpuidreader_dev(cpuidreader_dev&& other) noexcept
      : ctx_{std::exchange(other.ctx_, nullptr)} {}

  auto operator=(const cpuidreader_dev&) -> cpuidreader_dev& = delete;

  auto operator=(cpuidreader_dev&& other) noexcept -> cpuidreader_dev& {
    cpuidreader_dev{std::move(other)}.swap(*this);
    return *this;
  }

  ~cpuidreader_dev() = default;

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
  std::shared_ptr<detail::cpuid_dev_file> ctx_{};

  auto swap(cpuidreader_dev& other) noexcept -> void {
    std::swap(ctx_, other.ctx_);
  }
};

}  // namespace rjcp::cpuid

#endif
