#ifndef BENCHMARK_STRINTERN_OPTIONS_H
#define BENCHMARK_STRINTERN_OPTIONS_H

#include <filesystem>

#include "stdext/expected.h"

enum class strintern_impl {
  none,           //< No interning function.
  flist,          //< Forward list.
  set,            //< Use a set.
  unordered_set,  //< Use an unordered set.
};

/// @brief User options.
class options {
 public:
  options(const options&) = delete;
  auto operator=(const options&) -> options& = delete;
  options(options&&) = default;
  auto operator=(options&&) -> options& = default;
  ~options() = default;

  /// @brief The implementation to instantiate and test.
  ///
  /// @return the implementation to instantiate and test.
  [[nodiscard]] auto strintern() const noexcept -> strintern_impl {
    return mode_;
  }

  /// @brief The printable name of the mode.
  ///
  /// @return the printable name of the mode.
  [[nodiscard]] auto strintern_s() const noexcept -> const std::string& {
    return mode_s_;
  }

  /// @brief The path of the file to read for testing.
  ///
  /// @return a file system path (relative to the current directory) to read and
  /// test.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path& {
    return input_;
  }

 private:
  options() = default;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  friend auto make_options(int& argc, char* const argv[]) noexcept
      -> stdext::expected<options, int>;

  strintern_impl mode_{strintern_impl::none};
  std::string mode_s_{};
  std::filesystem::path input_{};
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
