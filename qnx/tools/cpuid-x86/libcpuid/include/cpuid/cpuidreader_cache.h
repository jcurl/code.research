#ifndef CPUID_CPUIDREADER_CACHE_H
#define CPUID_CPUIDREADER_CACHE_H

#include <memory>
#include <unordered_map>

#include "cpuid/cpuid.h"
#include "cpuid/cpuid_cache.h"

namespace rjcp::cpuid {

/// @brief A CPUID reader that caches the results of CPUID queries.
class cpuidreader_cache : public cpuidreader {
 public:
  cpuidreader_cache(cpuidreader& reader) : reader_{&reader} {};
  cpuidreader_cache(const cpuidreader_cache&) = delete;
  cpuidreader_cache(cpuidreader_cache&&) = default;
  auto operator=(const cpuidreader_cache&) -> cpuidreader_cache& = delete;
  auto operator=(cpuidreader_cache&&) -> cpuidreader_cache& = default;
  ~cpuidreader_cache() = default;

  /// @brief Checks if this class can query the CPUID.
  ///
  /// @return true if querying the CPUID should work, false otherwise.
  auto has_cpuid() -> bool override;

  /// @brief Indicates if this instance queries current hardware.
  ///
  /// @return true if queries are on current hardware, false if queries are
  /// cached and may not reflect this machine.
  auto is_online() -> bool override { return reader_ && reader_->is_online(); }

  /// @brief Get the number of cores available.
  ///
  /// @return the number of cores available.
  [[nodiscard]] auto cores() const -> unsigned int override {
    if (reader_) return reader_->cores();
    return cores_;
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
  /// Ensure that the object returned is destructed on the same thread where it
  /// is created.
  ///
  /// @param core The core to pin to.
  ///
  /// @return An optional object, if it has the value, represents the state of
  /// the CPU when querying the CPUID.
  auto enable_core(unsigned int core)
      -> stdext::expected<std::unique_ptr<cpuid_ctx>, int> override;

 protected:
  /// @brief Default constructor.
  ///
  /// Useful for derived classes that don't have an underlying cpuidreader (e.g.
  /// those that are reading from a file, then close the file when they're
  /// done).
  cpuidreader_cache() = default;

  /// @brief Set the number of cores that are being cached.
  ///
  /// Derived classes that don't have an underlying cpuidreader (e.g. those
  /// reading a file and using this class for caching the read file), would set
  /// the number of cores that are read.
  ///
  /// @param cores The number of cores that are supported by this cached
  /// instance.
  auto set_cores(unsigned int cores) { cores_ = cores; }

  /// @brief Check if there is a cached value for CPUID and return it.
  ///
  /// @param core The core to add for. Must be in the range of 0 .. cores() - 1.
  ///
  /// @param eax The input EAX register value.
  ///
  /// @param ecx The input ECX register value.
  ///
  /// @return the result if present, otherwise std::nullopt.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  auto get_cpuid(unsigned int core, cpuidreg eax, cpuidreg ecx)
      -> std::optional<cpuid_res>;

  /// @brief Add to the internal cache the results of a CPUID operation.
  ///
  /// @param core The core to add for. Must be in the range of 0 .. cores() - 1.
  ///
  /// @param eax The input EAX register value.
  ///
  /// @param ecx The input ECX register value.
  ///
  /// @param res The output result.
  ///
  /// @return the result if the value was added (or overwritten), true if the
  /// value is updated, false if the value was not inserted.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  auto add_cpuid(unsigned int core, cpuidreg eax, cpuidreg ecx, cpuid_res& res)
      -> bool;

 private:
  cpuidreader* reader_{};
  unsigned int cores_{0};
  std::unordered_map<unsigned int, cpuid_cache> cache_{};

  // The context is the current core.
  class core_ctx {
   public:
    core_ctx(unsigned int core) : core_{core} {}

    [[nodiscard]] auto core() const -> unsigned int { return core_; }

   private:
    unsigned int core_{};
  };
  std::shared_ptr<core_ctx> ctx_{};
};

}  // namespace rjcp::cpuid

#endif