#include "json_writer_context.h"

namespace ubench::sjson::detail {

auto json_writer_context::register_context(token_t token) -> token_t {
  if (closed_) throw json_writer_error("Context closed");
  if (!context_.empty() && token != context_.top())
    throw json_writer_error("Invalid context");

  token_t next = next_context_;
  next_context_++;

  context_.push(next);
  return next;
}

auto json_writer_context::unregister_context(token_t token) -> void {
  if (closed_) return;
  if (context_.empty()) throw json_writer_error("Context empty");
  if (token != context_.top()) throw json_writer_error("Invalid context");

  context_.pop();
}

auto json_writer_context::write(token_t token, std::string_view text) -> bool {
  if (closed_) return false;

  // If no context, or it isn't the current context, don't write.
  if (context_.empty()) throw json_writer_error("Invalid context");
  if (token != context_.top()) throw json_writer_error("Invalid context");

  ostream_ << text;
  return true;
}

auto json_writer_context::is_json_escape_char(char escape_char) -> char {
  switch (escape_char) {
    case 0x22:
      return 0x22;  // quotation mark
    case 0x5C:
      return 0x5C;  // reverse solidus
    case 0x2F:
      if (config_.escape_solidus) return 0x2F;
      return 0;
    case 0x08:
      return 0x62;  // backspace
    case 0x0C:
      return 0x66;  // form feed
    case 0x0A:
      return 0x6E;  // line feed
    case 0x0D:
      return 0x72;  // carriage return
    case 0x09:
      return 0x74;  // tab
    default:
      return 0;
  }
}

auto json_writer_context::json_escape_string(std::string_view str)
    -> std::string {
  std::string result;
  result.reserve(str.size());  // Pre-allocate to avoid reallocations

  size_t i = 0;
  while (i < str.size()) {
    auto escape_char = is_json_escape_char(str[i]);
    if (escape_char != 0) {
      // Character needs escaping
      result.push_back('\\');
      result.push_back(escape_char);
      ++i;
    } else {
      // Find the run of non-escaping characters
      size_t start = i;
      while (i < str.size() && is_json_escape_char(str[i]) == 0) {
        ++i;
      }
      // Append the entire run at once instead of push_back per character
      result.append(str.substr(start, i - start));
    }
  }

  return result;
}

}  // namespace ubench::sjson::detail
