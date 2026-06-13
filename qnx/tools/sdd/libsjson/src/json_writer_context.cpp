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

}  // namespace ubench::sjson::detail
