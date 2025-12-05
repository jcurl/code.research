#ifndef CPUID_CPUID_H
#define CPUID_CPUID_H

#include <cstdint>
#include <memory>
#include <optional>

#include "stdext/expected.h"

namespace rjcp::cpuid {

using cpuidreg = std::uint32_t;

/// @brief A CPUID Request.
struct cpuid_req {
  cpuidreg eax;  //< The leaf to query.
  cpuidreg ecx;  //< The subleaf to query.
};

/// @brief A CPUID response.
struct cpuid_res {
  cpuidreg eax;  //< The EAX register result.
  cpuidreg ebx;  //< The EBX register result.
  cpuidreg ecx;  //< The ECX register result.
  cpuidreg edx;  //< The EDX register result.
};

/// @brief A full CPUID request and response.
struct cpuid_info {
  struct cpuid_req req;  //< The CPUID request.
  struct cpuid_res res;  //< The CPUID response.
};

/// @brief A context class returned by a cpuidreader
///
/// When calling cpuidreader.cpuid, it returns a pointer to a context, that
/// while active causes the cpuidreader.cpuid() method to return data for a
/// specific CPU.
class cpuid_ctx {
 public:
  cpuid_ctx(const cpuid_ctx&) = delete;
  auto operator=(const cpuid_ctx&) -> cpuid_ctx& = delete;
  virtual ~cpuid_ctx() = default;

 protected:
  cpuid_ctx() = default;
  cpuid_ctx(cpuid_ctx&&) = default;
  auto operator=(cpuid_ctx&&) -> cpuid_ctx& = default;

 public:
  /// @brief The core that is pinned while this context is active.
  ///
  /// @return The core number that is currently active with this context.
  [[nodiscard]] virtual auto core() const -> unsigned int = 0;
};

/// @brief A CPUID reader class that can read for the default context, or a
/// pinned context.
///
/// There may be many ways to read CPUID information, though an instruction on
/// the CPU, or using an Operating System device driver, or a previously
/// captured dataset stored on disk somewhere.
class cpuidreader {
 public:
  cpuidreader(const cpuidreader&) = delete;
  auto operator=(const cpuidreader&) -> cpuidreader& = delete;
  virtual ~cpuidreader() = default;

 protected:
  cpuidreader() = default;
  cpuidreader(cpuidreader&&) = default;
  auto operator=(cpuidreader&&) -> cpuidreader& = default;

 public:
  /// @brief Checks if this class can query the CPUID.
  ///
  /// @return true if querying the CPUID should work, false otherwise.
  virtual auto has_cpuid() -> bool = 0;

  /// @brief Indicates if this instance queries current hardware.
  ///
  /// @return true if queries are on current hardware, false if queries are
  /// cached and may not reflect this machine.
  virtual auto is_online() -> bool = 0;

  /// @brief Get the number of cores available.
  ///
  /// @return the number of cores available.
  [[nodiscard]] virtual auto cores() const -> unsigned int = 0;

  /// @brief Query the CPUID for the register eax and ecx.
  ///
  /// @param eax The leaf to query for.
  ///
  /// @param ecx The subleaf to query for.
  ///
  /// @return the result of the query, or std::nullopt if the query failed.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  virtual auto cpuid(cpuidreg eax, cpuidreg ecx)
      -> std::optional<cpuid_res> = 0;

  /// @brief Enable CPUID instructions on the specified core, until object is
  /// destroyed.
  ///
  /// Ensure that there is only one active thread at a time. If there is a core
  /// already active when this method is called, the second call will fail.
  ///
  /// @param core The core to pin to.
  ///
  /// @return If the result is expected, it is a pointer to a context, that when
  /// the context goes out of scope, then querying the CPUID restores to default
  /// behaviour (e.g. the current thread might be such a default behaviour). If
  /// the result is unexpected, an error code is returned.
  virtual auto enable_core(unsigned int core)
      -> stdext::expected<std::unique_ptr<cpuid_ctx>, int> = 0;
};

template <class T, class... Args>
auto make_cpuidreader(Args&&... args) -> std::unique_ptr<T>;

}  // namespace rjcp::cpuid

#endif