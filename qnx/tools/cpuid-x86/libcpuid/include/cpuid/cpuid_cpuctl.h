#ifndef CPUID_CPUID_CPUCTL_H
#define CPUID_CPUID_CPUCTL_H

#include <optional>
#include <utility>

#include "cpuid/cpuid.h"

namespace rjcp::cpuid {

/// @brief Query the CPUID using usually the device '/dev/cpuctl' on FreeBSD.
class cpuid_cpuctl {
 public:
  cpuid_cpuctl() = default;

  /// @brief Instantiate the class with an opened fd.
  ///
  /// The file descriptor fd must be an opened (for read-only) to the path
  /// '/dev/cpuctlX' where X is the CPU number. The file descriptor fd is stored
  /// locally, so that the file must be opened at least for the lifetime of this
  /// class.
  ///
  /// @param core the file descriptor of the opened path '/dev/cpuctlX'.
  explicit cpuid_cpuctl(int fd) : fd_{fd} {}

  cpuid_cpuctl(const cpuid_cpuctl&) = delete;

  cpuid_cpuctl(cpuid_cpuctl&& other) noexcept
      : fd_{std::exchange(other.fd_, -1)} {}

  auto operator=(const cpuid_cpuctl&) -> cpuid_cpuctl& = delete;

  auto operator=(cpuid_cpuctl&& other) noexcept -> cpuid_cpuctl& {
    cpuid_cpuctl{std::move(other)}.swap(*this);
    return *this;
  }

  ~cpuid_cpuctl() = default;

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

 private:
  int fd_{-1};

  auto swap(cpuid_cpuctl& other) noexcept -> void { std::swap(fd_, other.fd_); }
};

}  // namespace rjcp::cpuid

#endif
