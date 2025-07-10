#ifndef BENCHMARK_CORE_OPTIONS_H
#define BENCHMARK_CORE_OPTIONS_H

#include <string>

#include "stdext/expected.h"

enum class core_mode {
  mode_readwrite,  //< Use sendto() for sending packets.
  mode_cas,        //< use sendmmsg() for sending packets.
};

/// @brief User options.
class options {
 public:
  options(const options&) = delete;
  auto operator=(const options&) -> options& = delete;
  options(options&&) = default;
  auto operator=(options&&) -> options& = default;
  ~options() = default;

  /// @brief Number of samples to obtain
  ///
  /// @return number of samples to run. Statistics is based on a sample.
  [[nodiscard]] auto samples() const noexcept -> unsigned int {
    return samples_;
  }

  /// @brief Number of iterations per sample.
  ///
  /// @return number of iterations per sample.
  [[nodiscard]] auto iters() const noexcept -> unsigned int { return iters_; }

  /// @brief The name of the benchmark (string) that the user provided.
  ///
  /// @return the name of the benchmark the user provided.
  [[nodiscard]] auto benchmark_name() const noexcept -> const std::string& {
    return benchmark_name_;
  }

  /// @brief Based on the benchmark_name(), the type of benchmark to run.
  ///
  /// @return the type of benchmark to run.
  [[nodiscard]] auto benchmark() const noexcept -> core_mode {
    return core_mode_;
  }

 private:
  options() = default;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  friend auto make_options(int argc, const char* const argv[]) noexcept
      -> stdext::expected<options, int>;

  std::string benchmark_name_{};
  core_mode core_mode_{};
  unsigned int samples_{500};
  unsigned int iters_{4000};
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
