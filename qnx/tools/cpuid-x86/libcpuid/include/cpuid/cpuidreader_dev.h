#ifndef CPUID_CPUIDREADER_DEV_H
#define CPUID_CPUIDREADER_DEV_H

#include <memory>
#include <optional>
#include <utility>

#include "cpuid/cpuid.h"
#include "cpuid/cpuid_dev.h"
#include "ubench/thread.h"

namespace rjcp::cpuid {

class cpuidreader_dev;

namespace detail {

/// @brief Manages the cpuid_dev object by opening the file.
class cpuid_dev_file {
 private:
  friend class rjcp::cpuid::cpuidreader_dev;

  // Only friends can instantiate with this token.
  class token {};

 public:
  /// @brief Open '/dev/cpu/<core>/cpuid'.
  ///
  /// If the file could not be opened, this object's boolean operator will
  /// return false. Get the error with the error(0 function.
  ///
  /// @param core The core to open.
  cpuid_dev_file(unsigned int core, token);

  cpuid_dev_file() = default;

  cpuid_dev_file(const cpuid_dev_file&) = delete;

  cpuid_dev_file(cpuid_dev_file&& other) noexcept
      : fd_{std::exchange(other.fd_, -1)},
        core_{std::exchange(other.core_, 0)},
        error_{std::exchange(other.error_, 0)},
        cpuid_{std::exchange(other.cpuid_, cpuid_dev{})} {}

  auto operator=(const cpuid_dev_file&) -> cpuid_dev_file& = delete;

  auto operator=(cpuid_dev_file&& other) noexcept -> cpuid_dev_file& {
    cpuid_dev_file{std::move(other)}.swap(*this);
    return *this;
  }

  /// @brief Release resources held by the class.
  ///
  /// Closes the file that is associated with the CPUID.
  ~cpuid_dev_file() { close(); };

  /// @brief Indicate if the file was opened and is queryable.
  operator bool() const { return fd_ != -1; };

  /// @brief Returns the error if this file isn't queryable.
  ///
  /// @return The errno value from the pin operation. If zero, the file was
  /// never opened.
  [[nodiscard]] auto error() const -> int { return error_; }

  /// @brief Get the core that this thread is pinned to.
  ///
  /// @return The core that is currently pinned by this class.
  [[nodiscard]] auto core() const -> unsigned int { return core_; }

  /// @brief Get the associated object to query the CPUID.
  ///
  /// @return The object to query the CPUID.
  [[nodiscard]] auto cpuid() -> cpuid_dev& { return cpuid_; }

  /// @brief Close the file associated with this class.
  ///
  /// @return true if closing was successful.
  auto close() -> bool;

 private:
  int fd_{-1};
  unsigned int core_{};
  int error_{};
  cpuid_dev cpuid_{};

  auto swap(cpuid_dev_file& other) noexcept -> void {
    std::swap(other.fd_, fd_);
    std::swap(other.core_, core_);
    std::swap(other.error_, error_);
    std::swap(other.cpuid_, cpuid_);
  }
};

/// @brief Context for querying the CPUID on a specific core.
///
/// The context is created and given to the user via the enable_core() function.
/// While the context is enabled, the reader will get the CPUID information.
/// When the context is destroyed, it closes the file for reading the CPUID.
class cpuid_dev_ctx : public cpuid_ctx {
 private:
  friend class rjcp::cpuid::cpuidreader_dev;

  // Only friends can instantiate with this token.
  class token {};

 public:
  cpuid_dev_ctx(std::shared_ptr<cpuid_dev_file> dev, token)
      : cpuid_file_{std::move(dev)} {};

  cpuid_dev_ctx(const cpuid_dev_ctx&) = delete;

  cpuid_dev_ctx(cpuid_dev_ctx&& other) noexcept
      : cpuid_file_{std::exchange(other.cpuid_file_, nullptr)} {}

  auto operator=(const cpuid_dev_ctx&) -> cpuid_dev_ctx& = delete;

  auto operator=(cpuid_dev_ctx&& other) noexcept -> cpuid_dev_ctx& {
    cpuid_dev_ctx{std::move(other)}.swap(*this);
    return *this;
  }

  /// @brief Closes the file associated with the CPUID.
  ///
  /// When destroying the object, the cpuid_dev class must still be in scope.
  ~cpuid_dev_ctx() {
    if (cpuid_file_ != nullptr) {
      cpuid_file_->close();
      cpuid_file_ = nullptr;
    }
  }

  /// @brief Get the core that is being queried.
  ///
  /// @return The core that is currently queried by this class.
  [[nodiscard]] auto core() const -> unsigned int override {
    return cpuid_file_ == nullptr ? 0 : cpuid_file_->core();
  }

 private:
  std::shared_ptr<cpuid_dev_file> cpuid_file_{};

  auto swap(cpuid_dev_ctx& other) noexcept -> void {
    std::swap(other.cpuid_file_, cpuid_file_);
  }
};

}  // namespace detail

/// @brief Query the CPUID using the device /dev/cpu/N/cpuid
class cpuidreader_dev : public cpuidreader {
 public:
  cpuidreader_dev() = default;

  cpuidreader_dev(const cpuidreader_dev&) = delete;

  cpuidreader_dev(cpuidreader_dev&& other) noexcept
      : default_{std::exchange(other.default_, false)},
        cpuid_file_{std::exchange(other.cpuid_file_, nullptr)} {}

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
  bool default_{false};

  // This must be a shared pointer, so that on move, the address of
  // cpuid_dev_file doesn't change. The enable_core creates a cpuid_dev_file for
  // the core, which is then given to a context. If we move the cpuidreader_dev,
  // the cpuid_file_ is moved, so it is no longer valid. The cpuid_dev_ctx
  // contains a pointer to the cpuid_dev_file. If that pointer is invalidated by
  // the move, we'll have a resource leak. Make it a unique_ptr, and the address
  // doesn't move, only the reference to the pointer does.
  //
  // Further, when moving this object, it may be now that after the move, this
  // object is destroyed before the context. Hence it must be shared, as the
  // lifetime of the cpuid_dev_ctx may be longer than this object.
  std::shared_ptr<detail::cpuid_dev_file> cpuid_file_{};

  auto swap(cpuidreader_dev& other) noexcept -> void {
    std::swap(default_, other.default_);
    std::swap(cpuid_file_, other.cpuid_file_);
  }
};

}  // namespace rjcp::cpuid

#endif
