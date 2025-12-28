#include "cpuid/cpuidreader_dev.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <memory>
#include <string>

#include "cpuid/cpuid_dev.h"
#include "stdext/expected.h"

namespace rjcp::cpuid {

namespace detail {

class cpuid_dev_file {
 public:
  /// @brief Open '/dev/cpu/<core>/cpuid'.
  ///
  /// If the file could not be opened, this object's boolean operator will
  /// return false. Get the error with the error(0 function.
  ///
  /// @param core The core to open.
  cpuid_dev_file(unsigned int core) {
    std::string path = "/dev/cpu/" + std::to_string(core) + "/cpuid";

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    fd_ = open(path.c_str(), O_RDONLY);
    if (fd_ < 0) {
      error_ = errno;
    }

    core_ = core;
    cpuid_ = cpuid_dev{fd_};
  }

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
  ~cpuid_dev_file() { close(); }

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
  auto close() -> bool {
    int result = 0;
    if (fd_ >= 0) {
      result = ::close(fd_);
      fd_ = -1;
    }

    return result == 0;
  }

  auto swap(cpuid_dev_file& other) noexcept -> void {
    std::swap(other.fd_, fd_);
    std::swap(other.core_, core_);
    std::swap(other.error_, error_);
    std::swap(other.cpuid_, cpuid_);
  }

 private:
  int fd_{-1};
  unsigned int core_{};
  int error_{};
  cpuid_dev cpuid_{};
};

}  // namespace detail

auto cpuidreader_dev::has_cpuid() -> bool {
  if (!ctx_ || !(*ctx_) || (ctx_.use_count() == 1 && ctx_->core() != 0)) {
    // Context doesn't exist, create a default; or context is internal only and
    // isn't the default, so create the default.
    ctx_ = std::make_shared<detail::cpuid_dev_file>(0);
  }

  return *ctx_;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto cpuidreader_dev::cpuid(cpuidreg eax, cpuidreg ecx)
    -> std::optional<cpuid_res> {
  if (!ctx_ || !(*ctx_) || (ctx_.use_count() == 1 && ctx_->core() != 0)) {
    // Context doesn't exist, create a default; or context is internal only and
    // isn't the default, so create the default.
    ctx_ = std::make_shared<detail::cpuid_dev_file>(0);
  }

  return ctx_->cpuid().cpuid(eax, ecx);
}

auto cpuidreader_dev::enable_core(unsigned int core)
    -> stdext::expected<std::unique_ptr<cpuid_ctx>, int> {
  if (ctx_ && *ctx_) {
    if (ctx_.use_count() > 1) {
      return stdext::unexpected{EINVAL};
    }

    if (ctx_->core() == core) {
      // No need to create a new context, use the existing one.
      return std::make_unique<detail::cpuid_basic_ctx<detail::cpuid_dev_file>>(
          ctx_);
    }

    // A different core than the current context, and no other context is in
    // use. Create a new one.
  }

  ctx_ = std::make_shared<detail::cpuid_dev_file>(core);
  if (ctx_ == nullptr) return stdext::unexpected{ENOMEM};
  if (!(*ctx_)) return stdext::unexpected{ctx_->error()};

  // When the user destroys the context, we don't automatically close the handle
  // to the underlying cpuid_dev object. This allows us to reuse it and keep it
  // open. If the user must make sure that the file descriptor given to
  // cpuid_dev is closed, they should also close the reader.
  return std::make_unique<detail::cpuid_basic_ctx<detail::cpuid_dev_file>>(
      ctx_);
}

}  // namespace rjcp::cpuid
