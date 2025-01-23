#ifndef UBENCH_ARGS_H
#define UBENCH_ARGS_H

#include <charconv>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ubench::string {

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

}  // namespace ubench::string

// This works, because the signature is the same. Compilers shouldn't complain
// if there is a redefinition.

/// @brief Copy src to string dst of size siz.
///
/// At most siz-1 characters will be copied.  Always NUL terminates (unless siz
/// == 0).
///
/// Note: this function is safe if only src is the same size or larger than dst.
/// If src is not NUL-terminated, out of bounds access can occur in src while
/// copying to dst. Use with std::string.c_str(), but not with
/// std::string_view.c_str().
///
/// @param dst The destination string to copy to.
///
/// @param src The source string to copy from.
///
/// @param size The maximum size available in dst.
///
/// @return The number of bytes strlen(src). If return value >= size, truncation
/// occurred.
extern "C" auto strlcpy(char *dst, const char *src, size_t size) -> size_t;

#endif