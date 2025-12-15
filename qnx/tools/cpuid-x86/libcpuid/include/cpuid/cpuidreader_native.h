#ifndef CPUID_CPUIDREADER_NATIVE_H
#define CPUID_CPUIDREADER_NATIVE_H

#include <memory>
#include <optional>
#include <thread>

#include "cpuid/cpuid.h"
#include "cpuid/cpuid_native.h"
#include "stdext/expected.h"
#include "ubench/thread.h"

namespace rjcp::cpuid {

class cpuidreader_native;

namespace detail {

/// @brief Context return for current thread for cpuidreader_native
class cpuid_native_ctx : public cpuid_ctx {
 private:
  friend class rjcp::cpuid::cpuidreader_native;

  // Only friends can instantiate with this token.
  class token {};

 public:
  cpuid_native_ctx() = delete;
  cpuid_native_ctx(ubench::thread::pin_core core, token)
      : core_{std::move(core)} {}
  cpuid_native_ctx(const cpuid_native_ctx&) = delete;
  cpuid_native_ctx(cpuid_native_ctx&&) = default;
  auto operator=(const cpuid_native_ctx&) -> cpuid_native_ctx& = delete;
  auto operator=(cpuid_native_ctx&&) -> cpuid_native_ctx& = default;
  ~cpuid_native_ctx() override = default;

  /// @brief Get the current core that is locked.
  ///
  /// @return The CPU core that is currently pinned.
  [[nodiscard]] auto core() const -> unsigned int override {
    return core_.core();
  }

 private:
  ubench::thread::pin_core core_;
};

}  // namespace detail

/// @brief Query the CPUID using the native CPUID instruction.
class cpuidreader_native : public cpuidreader {
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
  auto has_cpuid() -> bool override { return true; }

  /// @brief Indicates if this instance queries current hardware.
  ///
  /// @return true if queries are on current hardware, false if queries are
  /// cached and may not reflect this machine.
  auto is_online() -> bool override { return true; }

  /// @brief Get the number of cores available.
  ///
  /// @return the number of cores available.
  [[nodiscard]] auto cores() const -> unsigned int override {
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
  auto cpuid(cpuidreg eax, cpuidreg ecx) -> std::optional<cpuid_res> override {
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
      -> stdext::expected<std::unique_ptr<cpuid_ctx>, int> override {
    auto pinned = ubench::thread::pin_core(core);
    if (!pinned) return stdext::unexpected{pinned.error()};

    return std::make_unique<detail::cpuid_native_ctx>(
        std::move(pinned), detail::cpuid_native_ctx::token{});
  }

 private:
  cpuid_native cpuid_{};
};

}  // namespace rjcp::cpuid

#endif
