#ifndef BENCHMARK_LSHMEM_OPTIONS_H
#define BENCHMARK_LSHMEM_OPTIONS_H

#include <vector>

#include "stdext/expected.h"

/// @brief Decode the command line and present the options to the user.
class options {
 public:
  options(const options&) = delete;
  auto operator=(const options&) -> options& = delete;
  options(options&&) = default;
  auto operator=(options&&) -> options& = default;
  ~options() = default;

  /// @brief Get the verbosity level.
  ///
  /// Verbosity 0 is no verbosity. Increments by one every time the verbosity
  /// option is given.
  ///
  /// @return the verbosity level.
  [[nodiscard]] auto verbosity() const noexcept -> int { return verbosity_; }

  /// @brief The PIDs to print for, if given.
  ///
  /// A list of PIDs that should be scanned given on the command line. If empty,
  /// then should scan for all PIDs.
  ///
  /// @return the list of PIDs.
  [[nodiscard]] auto pids() const noexcept -> const std::vector<unsigned int>& {
    return pids_;
  }

  /// @brief If the groups of physical memory should be dumped.
  ///
  /// @return true if physical memory should be dumped.
  [[nodiscard]] auto phys_mem() const noexcept -> bool { return phys_mem_; }

  /// @brief if the groups of shared memory with other PIDs should be dumped.
  ///
  /// @return true if shared memory should be dumped.
  [[nodiscard]] auto shared_mem() const noexcept -> bool { return shared_mem_; }

  /// @brief if readable shared memory regions should also be dumped.
  ///
  /// @return true if readable shared memory regions should also be dumped.
  [[nodiscard]] auto readable() const noexcept -> bool { return read_; }

  /// @brief if we should print the typed memory sorted.
  ///
  /// @return true if typed memory should be dumped.
  [[nodiscard]] auto tymem() const noexcept -> bool { return tymem_; }

 private:
  options() = default;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  friend auto make_options(int argc, const char* const argv[]) noexcept
      -> stdext::expected<options, int>;

  int verbosity_{};
  std::vector<unsigned int> pids_{};
  bool phys_mem_{};
  bool shared_mem_{};
  bool read_{};
  bool tymem_{};
};

/// @brief Get options.
///
/// The command line options are parsed and the fields of this class are updated
/// accordingly. In case the user provides an error, or a value out of range for
/// an option, then this class will automatically tell the user of the error on
/// the console.
///
/// In cases where no option is provided, either a default value will be given
/// (as documented in the method), or zero will be returned indicating that no
/// option was provided.
///
/// If the user provides '-?' then help is printed.
///
/// @param argc [in, out] Reference to the number of arguments
///
/// @param argv [in, out] Pointer to the argument vector array
///
/// @return The options object, or an error code. An error code of zero
/// indicates no options, but the user requested help.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
[[nodiscard]] auto make_options(int argc, const char* const argv[]) noexcept
    -> stdext::expected<options, int>;

#endif
