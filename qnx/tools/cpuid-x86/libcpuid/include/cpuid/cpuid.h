#ifndef CPUID_CPUID_H
#define CPUID_CPUID_H

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "stdext/expected.h"

namespace rjcp::cpuid {

using cpuidreg = std::uint32_t;

/// @brief A CPUID Request.
struct cpuid_req {
  cpuidreg eax;  //< The leaf to query.
  cpuidreg ecx;  //< The subleaf to query.

  /// @brief Check equality for two cpuid_req objects.
  ///
  /// Needed so this can be used as a key in a map.
  ///
  /// @param other The object to compare to.
  ///
  /// @return true if the objects are value equivalent.
  auto operator==(const cpuid_req& other) const -> bool {
    return eax == other.eax && ecx == other.ecx;
  }
};

/// @brief A CPUID response.
struct cpuid_res {
  cpuidreg eax;  //< The EAX register result.
  cpuidreg ebx;  //< The EBX register result.
  cpuidreg ecx;  //< The ECX register result.
  cpuidreg edx;  //< The EDX register result.

  /// @brief Check equality for two cpuid_res objects.
  ///
  /// @param other The object to compare to.
  ///
  /// @return true if the objects are value equivalent.
  auto operator==(const cpuid_res& other) const -> bool {
    return eax == other.eax && ebx == other.ebx && ecx == other.ecx &&
           edx == other.edx;
  }
};

/// @brief A full CPUID request and response.
struct cpuid_info {
  struct cpuid_req req;  //< The CPUID request.
  struct cpuid_res res;  //< The CPUID response.

  /// @brief Check equality for two cpuid_info objects.
  ///
  /// @param other The object to compare to.
  ///
  /// @return true if the objects are value equivalent.
  auto operator==(const cpuid_info& other) const -> bool {
    return req == other.req && res == other.res;
  }
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

namespace detail {

/// @brief A context class that a cpuidreader class may return.
///
/// This class is not intended to be returned directly from a cpuidreader (but
/// the cpuid_ctx instead). It must be made in a public header that public class
/// definitions can refer to this class in their private block.
///
/// @tparam T The type the basic context class maintains. This must provide a
/// const method core() that returns an unsigned int.
template <typename T>
class cpuid_basic_ctx : public cpuid_ctx {
 public:
  /// @brief Construct the object for a core, with custom data.
  ///
  /// To use properly, the class maintaining the context creates a shared
  /// pointer to an object and keeps an internal reference of that. It then
  /// gives a copy of the shared pointer to this object, so that the reference
  /// count is now two. When the context class goes out of scope by the user, it
  /// is decremented by one. The class maintaining the context can then check if
  /// the context is now only one, and then it can free its own instance (as the
  /// object is now unique, and no external code can hold a context).
  ///
  /// If when this class is destroyed, an implementation should override this
  /// class and provide their own destructor cleaning up the resources (for
  /// example, so that the file is closed immediately when the user frees the
  /// context, not having to when some indeterminate amout of time that the
  /// reader maintaining this context checks and then frees the shared pointer).
  ///
  /// @param core The core number that is active with this context.
  ///
  /// @param value The value maintained by this object, which is a shared
  /// pointer.
  cpuid_basic_ctx(std::shared_ptr<T> value) : value_{std::move(value)} {}
  cpuid_basic_ctx(const cpuid_basic_ctx&) = delete;
  auto operator=(const cpuid_basic_ctx&) -> cpuid_basic_ctx& = delete;
  virtual ~cpuid_basic_ctx() = default;
  cpuid_basic_ctx(cpuid_basic_ctx&&) = default;
  auto operator=(cpuid_basic_ctx&&) -> cpuid_basic_ctx& = default;

  /// @brief The data maintained by this class.
  ///
  /// @return A reference to the data maintained (e.g. a CPUID class, or other
  /// data).
  [[nodiscard]] auto value() -> T& { return value_; }

  /// @brief The core that is pinned while this context is active.
  ///
  /// @return The core number that is currently active with this context.
  [[nodiscard]] auto core() const -> unsigned int override {
    return value_->core();
  }

 private:
  std::shared_ptr<T> value_{};
};

}  // namespace detail

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
auto make_cpuidreader(Args&&... args) -> std::unique_ptr<T> {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

/// @brief Dump all the CPUID registers for the CPU.
///
/// Interprets the CPUID registers to determine all registers that should be
/// dumped. The output of what registers are available are CPU specific.
///
/// @param reader a pointer to a cpuidreader that is used for the dump. It is
/// only used for the duration of the call. This can be obtained with
/// make_cpuidreader.
///
/// @param core the core to dump for, in the range of 0 to reader->cores().
///
/// @result A vector of all registers read.
auto cpuid_dump(cpuidreader& reader, unsigned int core)
    -> std::optional<std::vector<cpuid_info>>;

}  // namespace rjcp::cpuid

namespace std {

/// @brief Hash so the cpuid_req can be used as a key.
template <>
struct hash<rjcp::cpuid::cpuid_req> {
  auto operator()(const rjcp::cpuid::cpuid_req& k) const -> std::size_t {
    using std::hash;
    using std::size_t;

    // Combine hash values of individual members
    return hash<std::uint32_t>()(k.eax | k.ecx << 16);
  }
};

}  // namespace std

#endif
