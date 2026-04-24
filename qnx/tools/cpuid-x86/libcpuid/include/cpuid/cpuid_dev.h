#ifndef CPUID_CPUID_DEV_H
#define CPUID_CPUID_DEV_H

#include <optional>
#include <utility>

#include "cpuid/cpuid.h"

namespace rjcp::cpuid {

/// @brief Query the CPUID using usually the device '/dev/cpu/N/cpuid'.
class cpuid_dev {
 public:
  cpuid_dev() = default;

  /// @brief Instantiate the class with an opened fd.
  ///
  /// The file descriptor fd must be an opened (for read-only) to the path
  /// '/dev/cpu/N/cpuid', where N is the CPU number you're querying. The file
  /// descriptor fd is stored locally, so that the file must be opened at least
  /// for the lifetime of this class.
  ///
  /// @param core the file descriptor of the opened path '/dev/cpu/N/cpuid'.
  explicit cpuid_dev(int fd) : fd_{fd} {}

  cpuid_dev(const cpuid_dev&) = delete;

  cpuid_dev(cpuid_dev&& other) noexcept : fd_{std::exchange(other.fd_, -1)} {}

  auto operator=(const cpuid_dev&) -> cpuid_dev& = delete;

  auto operator=(cpuid_dev&& other) noexcept -> cpuid_dev& {
    cpuid_dev{std::move(other)}.swap(*this);
    return *this;
  }

  ~cpuid_dev() = default;

  /// @brief Checks if this class can query the CPUID.
  ///
  /// @return true if querying the CPUID should work, false otherwise.
  [[nodiscard]] auto has_cpuid() const -> bool { return fd_ >= 0; }

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

  auto swap(cpuid_dev& other) noexcept -> void { std::swap(fd_, other.fd_); }
};

}  // namespace rjcp::cpuid

#endif
