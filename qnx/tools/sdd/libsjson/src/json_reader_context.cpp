#include "json_reader_context.h"

#include <array>
#include <cassert>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace ubench::sjson::detail {

namespace {

auto is_digit(char c) noexcept -> bool { return c >= '0' && c <= '9'; }

struct keyword_entry {
  std::string_view text;
  json_string_keyword keyword;
};

constexpr std::array<keyword_entry, 3> keywords{{
    {"true", json_string_keyword::bool_true},
    {"false", json_string_keyword::bool_false},
    {"null", json_string_keyword::null_value},
}};

/// @brief Skips whitespace and '//' comments, which run to the end of the line.
/// @param json The json snippet, starting at the candidate skipped text.
/// @return The offset of the first character after the skipped text.
auto skip_whitespace_and_comments(std::string_view json) -> std::size_t {
  std::size_t pos{0};
  while (pos < json.size()) {
    if (std::isspace(static_cast<unsigned char>(json[pos]))) {
      ++pos;
    } else if (json.substr(pos, 2) == "//") {
      while (pos < json.size() && json[pos] != '\n' && json[pos] != '\r') {
        ++pos;
      }
    } else {
      break;
    }
  }
  return pos;
}

/// @brief Finds a keyword match in the keywords[].
/// @param json The json snippet, starting at the candidate keyword.
/// @return The keyword token if found, else std::nullopt.
auto find_keyword(std::string_view json) -> std::optional<json_reader_token> {
  for (const auto& entry : keywords) {
    if (json.substr(0, entry.text.size()) == entry.text)
      return json_reader_token{
          entry.keyword, json.substr(0, entry.text.size())};
  }
  return std::nullopt;
}

/// @brief Checks that text is well-formed UTF-8 (RFC 3629): no overlong forms,
/// no surrogates, and no code points above U+10FFFF.
/// @param text The bytes to check.
/// @return true if every sequence is valid.
auto is_valid_utf8(std::string_view text) -> bool {
  std::size_t i{};
  while (i < text.size()) {
    const auto lead = static_cast<unsigned char>(text[i]);
    if (lead < 0x80) {
      ++i;
      continue;
    }

    // The valid range for the second byte. Later bytes are 80-BF.
    std::size_t length{};
    unsigned char second_lo{0x80};
    unsigned char second_hi{0xBF};
    if (lead >= 0xC2 && lead <= 0xDF) {
      length = 2;
    } else if (lead >= 0xE0 && lead <= 0xEF) {
      length = 3;
      if (lead == 0xE0) second_lo = 0xA0;
      if (lead == 0xED) second_hi = 0x9F;  // Excludes surrogates.
    } else if (lead >= 0xF0 && lead <= 0xF4) {
      length = 4;
      if (lead == 0xF0) second_lo = 0x90;
      if (lead == 0xF4)
        second_hi = 0x8F;  // Excludes code points above U+10FFFF.
    } else {
      return false;
    }

    if (text.size() - i < length) return false;
    for (std::size_t k{1}; k < length; ++k) {
      const auto b = static_cast<unsigned char>(text[i + k]);
      const auto lo = (k == 1) ? second_lo : static_cast<unsigned char>(0x80);
      const auto hi = (k == 1) ? second_hi : static_cast<unsigned char>(0xBF);
      if (b < lo || b > hi) return false;
    }
    i += length;
  }
  return true;
}

/// @brief Finds a quoted string at the start of the json snippet.
/// @param json The json snippet, starting at the candidate string.
/// @return The string content without its delimiters, else std::nullopt if
/// there is no closing quote.
auto find_string(std::string_view json) -> std::optional<json_reader_token> {
  if (json.empty() || (json[0] != '"' && json[0] != '\'')) return std::nullopt;

  char quote{json[0]};
  std::size_t end{1};
  while (end < json.size() && json[end] != quote) {
    end += (json[end] == '\\') ? 2 : 1;
  }
  if (end >= json.size()) return std::nullopt;
  return json_reader_token{
      json_string_keyword::string, json.substr(1, end - 1)};
}

/// @brief Finds an integer at the start of the json snippet. Floats are not
/// supported.
/// @param json The json snippet, starting at the candidate integer.
/// @return The integer including any sign, else std::nullopt.
auto find_integer(std::string_view json) -> std::optional<json_reader_token> {
  std::size_t end{0};
  if (!json.empty() && (json[0] == '+' || json[0] == '-')) {
    ++end;
  }

  std::size_t digits_start{end};
  while (end < json.size() && is_digit(json[end])) {
    ++end;
  }
  if (end == digits_start) return std::nullopt;

  if (end < json.size() &&
      (json[end] == '.' || json[end] == 'e' || json[end] == 'E'))
    return std::nullopt;

  return json_reader_token{json_string_keyword::integer, json.substr(0, end)};
}

/// @brief Finds a single-character punctuation token at the start of the json
/// snippet.
/// @param json The json snippet, starting at the candidate punctuation.
/// @return The punctuation token, else std::nullopt.
auto find_char(std::string_view json) -> std::optional<json_reader_token> {
  if (json.empty()) return std::nullopt;

  switch (json[0]) {
    case '[':
      return json_reader_token{
          json_string_keyword::array_open, json.substr(0, 1)};
    case ']':
      return json_reader_token{
          json_string_keyword::array_close, json.substr(0, 1)};
    case '{':
      return json_reader_token{
          json_string_keyword::object_open, json.substr(0, 1)};
    case '}':
      return json_reader_token{
          json_string_keyword::object_close, json.substr(0, 1)};
    case ':':
      return json_reader_token{
          json_string_keyword::object_colon, json.substr(0, 1)};
    case ',':
      return json_reader_token{json_string_keyword::comma, json.substr(0, 1)};
    default:
      return std::nullopt;
  }
}

/// @brief Converts the text of an integer token to a json_int.
/// @param text The integer including any sign, as produced by find_integer().
/// @return The json_int, or std::nullopt if the value does not fit in
/// std::int64_t.
auto to_integer(std::string_view text) -> std::optional<json_int> {
  // std::from_chars does not accept a leading '+'.
  if (!text.empty() && text[0] == '+') text.remove_prefix(1);

  std::int64_t value{};
  const char* first{text.data()};
  const char* last{std::next(first, static_cast<std::ptrdiff_t>(text.size()))};
  const auto [ptr, ec] = std::from_chars(first, last, value);
  if (ec != std::errc{} || ptr != last) return std::nullopt;
  return json_int{value};
}

/// @brief Gets the value of a hexadecimal digit.
/// @param c The character to convert.
/// @return The value 0-15, else std::nullopt if c is not a hexadecimal digit.
auto hex_value(char c) noexcept -> std::optional<char32_t> {
  if (c >= '0' && c <= '9') return static_cast<char32_t>(c - '0');
  if (c >= 'a' && c <= 'f') return static_cast<char32_t>(c - 'a' + 10);
  if (c >= 'A' && c <= 'F') return static_cast<char32_t>(c - 'A' + 10);
  return std::nullopt;
}

/// @brief Reads four hexadecimal digits, as in a \uXXXX escape.
/// @param text The string content.
/// @param pos The index of the first digit.
/// @return The 16-bit value, else std::nullopt if there are fewer than four
/// characters or any of them is not a hexadecimal digit.
auto read_hex4(std::string_view text, std::size_t pos)
    -> std::optional<char32_t> {
  if (pos > text.size() || text.size() - pos < 4) return std::nullopt;

  char32_t value{};
  for (std::size_t i{pos}; i < pos + 4; ++i) {
    const auto digit = hex_value(text[i]);
    if (!digit) return std::nullopt;
    value = (value << 4) | *digit;
  }
  return value;
}

/// @brief Appends the UTF-8 encoding of a Unicode code point.
/// @param out The string to append to.
/// @param cp The code point, at most U+10FFFF and not a surrogate.
auto append_utf8(std::string& out, char32_t cp) -> void {
  if (cp < 0x80) {
    out += static_cast<char>(cp);
  } else if (cp < 0x800) {
    out += static_cast<char>(0xC0 | (cp >> 6));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    out += static_cast<char>(0xE0 | (cp >> 12));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else {
    out += static_cast<char>(0xF0 | (cp >> 18));
    out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  }
}

/// @brief Converts the content of a JSON string to a json_string.
///
/// Content without escapes is referenced, not copied. Otherwise the escapes
/// are decoded into an owned string. Surrogate pairs are combined.
///
/// @param text The string content between the quotes.
/// @return The json_string, else std::nullopt if the raw content is not
/// well-formed UTF-8 or an escape is malformed.
auto to_string(std::string_view text) -> std::optional<json_string> {
  if (!is_valid_utf8(text)) return std::nullopt;
  if (text.find('\\') == std::string_view::npos) return json_string{text};

  std::string out;
  out.reserve(text.size());
  for (std::size_t i{}; i < text.size(); ++i) {
    if (text[i] != '\\') {
      out += text[i];
      continue;
    }

    if (++i >= text.size()) return std::nullopt;
    switch (text[i]) {
      case 'b':
        out += '\b';
        break;
      case 'f':
        out += '\f';
        break;
      case 'n':
        out += '\n';
        break;
      case 'r':
        out += '\r';
        break;
      case 't':
        out += '\t';
        break;
      case 'v':
        out += '\v';
        break;
      case 'u': {
        const auto unit = read_hex4(text, i + 1);
        if (!unit) return std::nullopt;
        i += 4;

        char32_t cp{*unit};
        if (*unit >= 0xD800 && *unit <= 0xDBFF) {
          // A high surrogate must be followed by an escaped low surrogate.
          if (text.substr(i + 1, 2) != "\\u") return std::nullopt;
          const auto low = read_hex4(text, i + 3);
          if (!low || *low < 0xDC00 || *low > 0xDFFF) return std::nullopt;
          cp = 0x10000 + ((*unit - 0xD800) << 10) + (*low - 0xDC00);
          i += 6;
        } else if (*unit >= 0xDC00 && *unit <= 0xDFFF) {
          return std::nullopt;
        }
        append_utf8(out, cp);
        break;
      }
      default:
        // '"', '\\', '/' and '\'' map to themselves, as do other escaped
        // characters.
        out += text[i];
        break;
    }
  }
  return json_string{std::move(out)};
}

}  // namespace

// The following BNF should be implemented for this variant.
//
// <root> := <container>
// <json> := <container> | <value>
// <container> := <object> | <array>
// <value> := integer | null_value | bool_true | bool_false | string
// <object> := object_open ( <member> ( comma <member> )* comma? ) object_close
// <member> := string object_colon <json>
// <array> := array_open ( <json> ( comma <json> )* comma? ) array_close

auto json_reader_context::get_keyword() -> std::optional<json_reader_token> {
  // Records a parsing error and returns std::nullopt.
  const auto fail = [this]() -> std::optional<json_reader_token> {
    error_ = true;
    return std::nullopt;
  };

  std::size_t offset = skip_whitespace_and_comments(json_.substr(json_pos_));
  if (json_pos_ + offset >= json_.size()) {
    json_pos_ = json_.size();
    return std::nullopt;
  }

  json_pos_ += offset;
  std::size_t start{json_pos_};

  // Moves past 'consumed' characters from 'start' and returns the token.
  const auto take = [this, start](json_reader_token token,
                        std::size_t consumed) -> json_reader_token {
    json_pos_ = start + consumed;
    return token;
  };

  if (auto punct = find_char(json_.substr(start))) {
    return take(*punct, punct->value().size());
  }

  if (auto str = find_string(json_.substr(start))) {
    // The content plus the opening and closing delimiters.
    return take(*str, str->value().size() + 2);
  }

  if (auto entry = find_keyword(json_.substr(start))) {
    return take(*entry, entry->value().size());
  }

  if (auto num = find_integer(json_.substr(start))) {
    return take(*num, num->value().size());
  }

  return fail();
}

auto json_reader_context::get_token() -> std::optional<json_member> {
  if (closed_ || error_) return std::nullopt;

  auto token = get_keyword();
  while (true) {
    if (!token) {
      if (depth() > 0) set_error();  // Input ended inside a block.
      return std::nullopt;
    }

    if (depth() == 0) {
      // We're at the root. We expect only a container.
      if (token->keyword() == json_string_keyword::array_open ||
          token->keyword() == json_string_keyword::object_open) {
        stack_enter(token->keyword());
        state_start_block_ = true;
        return to_token(*token, shared_from_this());
      }
      set_error();  // Unexpected keyword.
      return std::nullopt;
    }

    if (is_array()) {
      // Expect "( <json> ( comma <json> )* comma? ) array_close"
      if (state_start_block_) {
        // Next must be either a <json> or array_close.
        if (token->keyword() == json_string_keyword::array_close) {
          stack_leave(token->keyword());
          if (depth() == 0) {
            close_root();
            return std::nullopt;
          }
          state_start_block_ = false;
          // This <array> is finished. The enclosing block resumes with a
          // comma or close, so end this block here without consuming more.
          return std::nullopt;
        } else if (is_keyword_container(token->keyword())) {
          stack_enter(token->keyword());
          state_start_block_ = true;
          auto t = to_token(*token, shared_from_this());
          assert(t.has_value());
          return t;
        } else if (is_keyword_value(token->keyword())) {
          state_start_block_ = false;
          auto t = to_token(*token);
          if (!t) set_error();  // Value does not convert, e.g. out of range.
          return t;
        } else {
          set_error();  // Unexpected keyword.
          return std::nullopt;
        }
      } else {
        // Next must be either a comma, or array_close
        if (token->keyword() == json_string_keyword::comma) {
          token = get_keyword();
        } else if (token->keyword() != json_string_keyword::array_close) {
          set_error();  // Unexpected keyword.
          return std::nullopt;
        }
        state_start_block_ = true;
        continue;
      }
    } else if (is_object()) {
      // Expect "( <member> ( comma <member> )* comma? ) object_close"
      if (state_start_block_) {
        // Next must be either a <json> or array_close.
        if (token->keyword() == json_string_keyword::object_close) {
          stack_leave(token->keyword());
          if (depth() == 0) {
            close_root();
            return std::nullopt;
          }
          state_start_block_ = false;
          // This <object> is finished. The enclosing block resumes with a
          // comma or close, so end this block here without consuming more.
          return std::nullopt;
        }

        if (token->keyword() != json_string_keyword::string) {
          set_error();  // Need a string for the member name.
          return std::nullopt;
        }
        auto member = to_string(token->value());
        if (!member.has_value()) {
          set_error();  // Error in the member name.
          return std::nullopt;
        }

        token = get_keyword();
        if (!token) {
          set_error();  // Input ended inside a member.
          return std::nullopt;
        }
        if (token->keyword() != json_string_keyword::object_colon) {
          set_error();  // Unexpected keyword.
          return std::nullopt;
        }

        token = get_keyword();
        if (!token) {
          set_error();  // Input ended inside a member.
          return std::nullopt;
        }
        if (is_keyword_container(token->keyword())) {
          stack_enter(token->keyword());
          state_start_block_ = true;
          auto t = to_token(*token, shared_from_this());
          if (!t) {
            set_error();  // Container token does not convert.
            return std::nullopt;
          }
          return json_key{*member, *t};
        } else if (is_keyword_value(token->keyword())) {
          state_start_block_ = false;
          auto t = to_token(*token);
          if (!t) {
            set_error();  // Value does not convert, e.g. out of range.
            return std::nullopt;
          }
          return json_key{*member, *t};
        }
        set_error();  // Unexpected keyword.
        return std::nullopt;
      } else {
        // Next must be either a comma, or object_close
        if (token->keyword() == json_string_keyword::comma) {
          token = get_keyword();
        } else if (token->keyword() != json_string_keyword::object_close) {
          set_error();  // Unexpected keyword.
          return std::nullopt;
        }
        state_start_block_ = true;
        continue;
      }
    }
    assert(false);
    return std::nullopt;
  }
}

/// @brief Finish parsing after the root container has closed.
///
/// Only whitespace and comments may follow. Any other token, or a tokenizer
/// error, sets an error. Then the context is closed.
auto json_reader_context::close_root() -> void {
  auto next = get_keyword();
  if (next) set_error();  // Extraneous data after the root.
  close();
}

auto json_reader_context::skip_block() -> void {
  if (closed_ || error_ || depth() == 0) return;

  const auto target = depth() - 1;
  while (depth() > target) {
    const auto before = depth();
    if (!get_token() && depth() == before) {
      // No block boundary was crossed: the input ended or an error occurred.
      if (!has_error()) set_error();  // Input ended inside the block.
      return;
    }
  }
}

auto json_reader_context::skip_container(
    std::size_t depth, const json_token& token) -> bool {
  if (this->depth() != depth + 1 || is_owned()) return false;
  if (!std::holds_alternative<json_array>(token) &&
      !std::holds_alternative<json_object>(token)) {
    return false;
  }
  skip_block();
  return true;
}

auto to_token(json_reader_token& token) -> std::optional<json_token> {
  switch (token.keyword()) {
    case json_string_keyword::integer:
      if (auto value = to_integer(token.value())) return json_token{*value};
      return std::nullopt;
    case json_string_keyword::string:
      if (auto value = to_string(token.value())) return json_token{*value};
      return std::nullopt;
    case json_string_keyword::bool_true:
      return json_token{json_bool{true}};
    case json_string_keyword::bool_false:
      return json_token{json_bool{false}};
    case json_string_keyword::null_value:
      return json_token{json_null{}};
    default:
      return std::nullopt;
  }
}

auto to_token(json_reader_token& token,
    std::shared_ptr<json_reader_context> context) -> std::optional<json_token> {
  switch (token.keyword()) {
    case json_string_keyword::array_open:
      return json_token{json_array{std::move(context)}};
    case json_string_keyword::object_open:
      return json_token{json_object{std::move(context)}};
    default:
      return to_token(token);
  }
}

}  // namespace ubench::sjson::detail
