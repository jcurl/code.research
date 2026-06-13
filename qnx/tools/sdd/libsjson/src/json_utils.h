#ifndef SJSON_JSON_UTILS_H
#define SJSON_JSON_UTILS_H

#include <string>
#include <string_view>

namespace ubench::sjson::detail {

/// @brief Tests if a character is an escape character.
///
/// @param escape_char the character to test for. As all escape characters are
/// in the first code page of unicode (0x00-0x7F), a character is sufficient.
/// Should a unicode character be out of this range, it is not an escape
/// character.
///
/// @return the character to escape with (e.g. `\` . return_value), or 0 if not
/// an escape character.
auto is_json_escape_char(char escape_char) -> char;

/// @brief Produce a properly quoted string, assuming the input is valid UTF8.
///
/// @param str the input string to quote
///
/// @return the quoted output string that can be written to a stream.
auto json_escape_string(std::string_view str) -> std::string;

}  // namespace ubench::sjson::detail

#endif
