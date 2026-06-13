#include "sjson/json_writer.h"
#include "json_utils.h"
#include "json_writer_context.h"

namespace ubench::sjson {

json_writer_object::json_writer_object(
    detail::token_t t, std::shared_ptr<detail::json_writer_context> context)
    : token_{t}, context_{std::move(context)} {
  context_->write(token_, "{");
}

auto json_writer_object::write_object(std::string_view key)
    -> json_writer_object {
  write_new_field(key);
  detail::token_t new_token = context_->register_context(token_);
  return {new_token, context_};
}

auto json_writer_object::write_array(std::string_view key)
    -> json_writer_array {
  write_new_field(key);
  detail::token_t new_token = context_->register_context(token_);
  return {new_token, context_};
}

auto json_writer_object::write_value(std::string_view key, const char* value)
    -> void {
  if (value == nullptr) {
    write_null(key);
  } else {
    write_value_quoted(key, std::string_view(value));
  }
}

auto json_writer_object::write_value(
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    std::string_view key, std::string_view value) -> void {
  write_value_quoted(key, value);
}

auto json_writer_object::write_value(std::string_view key, bool value) -> void {
  write_new_field(key);
  if (value) {
    context_->write(token_, "true");
  } else {
    context_->write(token_, "false");
  }
}

auto json_writer_object::write_null(std::string_view key) -> void {
  write_new_field(key);
  context_->write(token_, "null");
}

auto json_writer_object::write_new_field(std::string_view key) -> void {
  if (has_element_) {
    context_->write(token_, ", \"");
  } else {
    context_->write(token_, " \"");
    has_element_ = true;
  }

  context_->write(token_, detail::json_escape_string(key));
  context_->write(token_, "\": ");
}

auto json_writer_object::write_value_unquoted(
    std::string_view key, std::string value) -> void {
  write_new_field(key);
  context_->write(token_, value);
}

auto json_writer_object::write_value_quoted(
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    std::string_view key, std::string_view value) -> void {
  write_new_field(key);
  context_->write(token_, "\"");
  context_->write(token_, detail::json_escape_string(value));
  context_->write(token_, "\"");
}

auto json_writer_object::close() -> void {
  if (context_) {
    context_->write(token_, " }");
    context_->unregister_context(token_);
  }
  context_.reset();
}

}  // namespace ubench::sjson
