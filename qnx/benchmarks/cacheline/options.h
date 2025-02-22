#ifndef BENCHMARK_CACHELINE_OPTIONS_H
#define BENCHMARK_CACHELINE_OPTIONS_H

#include "stdext/expected.h"

/// @brief User options.
class options {
 public:
  options(const options&) = delete;
  auto operator=(const options&) -> options& = delete;
  options(options&&) = default;
  auto operator=(options&&) -> options& = default;
  ~options() = default;

  /// @brief The size of the array buffer for the test.
  ///
  /// The array buffer should be large enough that when using strides, it is
  /// larger than the size of the largest cache of the CPU. More modern CPUs
  /// have large caches.
  ///
  /// @return the size of the array buffer for the test.
  [[nodiscard]] auto buffer_size() const noexcept -> unsigned int {
    return buffer_size_;
  }

 private:
  options() = default;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  friend auto make_options(int& argc, char* const argv[]) noexcept
      -> stdext::expected<options, int>;

  unsigned int buffer_size_{256};
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
[[nodiscard]] auto make_options(int& argc, char* const argv[]) noexcept
    -> stdext::expected<options, int>;

#endif