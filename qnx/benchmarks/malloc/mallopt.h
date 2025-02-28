#ifndef BENCHMARK_MALLOPT_H
#define BENCHMARK_MALLOPT_H

#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "stdext/expected.h"

/// @brief User options.
class mallopt_options {
 public:
  mallopt_options(const mallopt_options&) = delete;
  auto operator=(const mallopt_options&) -> mallopt_options& = delete;
  mallopt_options(mallopt_options&&) = default;
  auto operator=(mallopt_options&&) -> mallopt_options& = default;
  ~mallopt_options() = default;

  /// @brief If should lock all.
  ///
  /// @return if should lock all.
  [[nodiscard]] auto mlock_all() const noexcept -> bool { return mlock_all_; }

  /// @brief The list of mallopts and the values to set.
  ///
  /// @return a reference to a list of mallopts and the values that should be
  /// set. This is a tuple of {mallopt, value, key_name}.
  [[nodiscard]] auto mallopts() const noexcept
      -> const std::vector<std::tuple<int, int, std::string>>& {
    return mallopts_;
  }

  /// @brief Get the maximum amount of memory to alloc in the benchmark range.
  ///
  /// @return the maximum amount of memory to alloc in the benchmark range.
  [[nodiscard]] auto max_malloc() const noexcept -> unsigned int {
    return max_;
  }

 private:
  mallopt_options() = default;

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  friend auto make_options(int& argc, char* const argv[]) noexcept
      -> stdext::expected<mallopt_options, int>;

  bool mlock_all_{};
  unsigned int max_{1 << 30};
  std::vector<std::tuple<int, int, std::string>> mallopts_{};
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
    -> stdext::expected<mallopt_options, int>;

namespace impl {

// implemented in files mallopt_*.cpp

auto parse_mallopt_arg(std::string_view mallopt_arg)
    -> std::vector<std::tuple<int, int, std::string>>;
auto print_mallopt_help() -> void;

}  // namespace impl

#endif
