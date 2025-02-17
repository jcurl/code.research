#ifndef UBENCH_ARGS_H
#define UBENCH_ARGS_H

#include <array>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

namespace ubench::string {

/// @brief Parses the string as a number.
///
/// Note, only integer types are supported. On QNX and Clang cmopilers, floating
/// point types are not supported.
///
/// @tparam T The type to convert from the string to.
///
/// @param arg The argument given to the program which is to be parsed to an
/// integer.
///
/// @return The value that was read, or no value if there was a parsing error.
template <typename T, std::enable_if_t<std::is_integral<T>{}, bool> = true>
auto parse_int(std::string_view arg) -> std::optional<T> {
  T value;
  auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);
  if (ec != std::errc{}) return {};
  if (ptr != arg.data() + arg.size()) return {};
  return value;
}

/// @brief Split an argument with commas into individual elements.
///
/// If the string is empty, then a single argument is returned, but it is an
/// empty vector.
///
/// @param arg the argument to split.
///
/// @param fields the maximum number of splits to make. Elements after this
/// split just belong to the last field. If set to zero then there is no upper
/// limit on the fields.
///
/// @return a vector of split arguments.
auto split_args(std::string_view arg, unsigned int fields = 0)
    -> std::vector<std::string_view>;

/// @brief Split an argument with commas into individual elements.
///
/// If the string is empty, then a single argument is returned, but it is an
/// empty vector.
///
/// @param arg the argument to split.
///
/// @return a vector of split arguments converted to the type requested.
template <typename T, std::enable_if_t<std::is_integral<T>{}, bool> = true>
auto split_args_int(std::string_view arg) -> std::vector<T> {
  std::vector<T> split_args{};

  std::string::size_type sz = 0;
  while (true) {
    std::string_view::size_type next = arg.find_first_of(',', sz);
    if (next == std::string_view::npos) {
      auto v = parse_int<T>(std::string_view{arg.data() + sz, arg.size() - sz});
      if (!v) return {};
      split_args.emplace_back(*v);
      return split_args;
    }

    auto v = parse_int<T>(std::string_view{arg.data() + sz, next - sz});
    if (!v) return {};
    split_args.emplace_back(*v);
    sz = next + 1;
  }
}

/// @brief convert a hex string to an integer.
///
/// Converts the string containing a hex number to an integer. It will continue
/// to convert until there is a non-hex character to convert. On overlow, the
/// higher bits will be removed, effectively casting the result to the lowest
/// bits.
///
/// This function is to allow optimisations for speed.
///
/// @tparam T the integer type parameter to convert to.
///
/// @param first valid character range to parse from.
///
/// @param last valid character range to parse to.
///
/// @param value the out-parameter where the parsed value is stored if
/// successful.
///
/// @return the value of the string converted to an integer.
template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
auto from_chars_hex(const char *first, const char *last, T &value)
    -> std::from_chars_result {
  static constexpr std::array<std::uint8_t, 256> hexval{
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 00 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      0F
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 10 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      1F
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 20 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      2F
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  // 30 -
      0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      3F
      0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,  // 40 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      4F
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 50 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      5F
      0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,  // 60 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      6F
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 70 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      7F
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 80 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      8F
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 90 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      9F
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // A0 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      AF
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // B0 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      BF
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // C0 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      CF
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // D0 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      DF
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // E0 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      EF
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // F0 -
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //      FF
  };
  value = 0;
  std::uint8_t v{};
  do {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    v = hexval[static_cast<uint8_t>(*first)];
    if (v == 0xFF) return {first, std::errc::invalid_argument};
    value = (value << 4) | v;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    first++;
  } while (v != 0xFF && first != last);
  return {first, std::errc{}};
}

/// @brief Print a standard error string format.
///
/// @param err the error number given by the Operating System.
///
/// @return a string in the form of 'string description (errno)'.
auto perror(int err) -> std::string;

/// @brief Print a standard error string format.
///
/// @return a string in the form of 'string description (errno)'.
auto perror(const std::string &msg) -> void;

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