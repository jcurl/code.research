#ifndef UBENCH_ARGS_H
#define UBENCH_ARGS_H

#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ubench::args {

/// @brief Split an argument with commas into individual elements.
///
/// If the string is empty, then a single argument is returned, but it is an
/// empty string.
///
/// @param arg the argument to split.
///
/// @return a vector of split arguments.
auto split_args(std::string_view arg)
    -> std::optional<std::vector<std::string>>;

/// @brief Parses the string as an unsigned integer.
///
/// @tparam T The type to convert from the string to.
///
/// @param arg The argument given to the program which is to be parsed to an
/// integer.
///
/// @return The value that was read, or no value if there was a parsing error.
template <typename T>
auto parse_int(std::string_view arg) -> std::optional<T> {
  T value;
  auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);
  if (ec != std::errc{}) return {};
  if (ptr != arg.data() + arg.size()) return {};
  return value;
}

}  // namespace ubench::args

#endif