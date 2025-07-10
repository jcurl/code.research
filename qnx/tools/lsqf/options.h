#ifndef LSQF_OPTIONS_H
#define LSQF_OPTIONS_H

#include <vector>

#include "stdext/expected.h"

/// @brief User options.
class options {
 public:
  options(const options&) = delete;
  auto operator=(const options&) -> options& = delete;
  options(options&&) = default;
  auto operator=(options&&) -> options& = default;
  ~options() = default;

  /// @brief If the output should show sidechannel connections.
  ///
  /// @return true if the output should show sidechannel connections.
  [[nodiscard]] auto show_sidechannels() const noexcept -> bool {
    return show_sidechannels_;
  }

  /// @brief If the output should show dead connections.
  ///
  /// @return true if the output should show dead connections.
  [[nodiscard]] auto show_dead() const noexcept -> bool { return show_dead_; }

  /// @brief If the output should show internal (self) connections.
  ///
  /// @return true if the output should show internal connections.
  [[nodiscard]] auto show_self() const noexcept -> bool { return show_self_; }

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

 private:
  options() = default;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  friend auto make_options(int argc, const char* const argv[]) noexcept
      -> stdext::expected<options, int>;

  bool show_sidechannels_{};
  bool show_dead_{};
  bool show_self_{};
  int verbosity_{};
  std::vector<unsigned int> pids_{};
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
