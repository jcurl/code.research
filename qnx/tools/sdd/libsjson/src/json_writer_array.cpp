#include "sjson/json_writer.h"
#include "json_utils.h"
#include "json_writer_context.h"

namespace ubench::sjson {

json_writer_array::json_writer_array(
    detail::token_t t, std::shared_ptr<detail::json_writer_context> context)
    : token_{t}, context_{std::move(context)} {
  context_->write(token_, "[");
}

auto json_writer_array::write_object() -> json_writer_object {
  write_new_field();
  detail::token_t new_token = context_->register_context(token_);
  return {new_token, context_};
}

auto json_writer_array::write_array() -> json_writer_array {
  write_new_field();
  detail::token_t new_token = context_->register_context(token_);
  return {new_token, context_};
}

auto json_writer_array::write_value(const char* value) -> void {
  if (value == nullptr) {
    write_null();
  } else {
    write_value_quoted(std::string_view(value));
  }
}

auto json_writer_array::write_value(std::string_view value) -> void {
  write_value_quoted(value);
}

auto json_writer_array::write_value(bool value) -> void {
  write_new_field();
  if (value) {
    context_->write(token_, "true");
  } else {
    context_->write(token_, "false");
  }
}

auto json_writer_array::write_null() -> void {
  write_new_field();
  context_->write(token_, "null");
}

auto json_writer_array::write_new_field() -> void {
  if (has_element_) {
    context_->write(token_, ", ");
  } else {
    context_->write(token_, " ");
    has_element_ = true;
  }
}

auto json_writer_array::write_value_unquoted(std::string value) -> void {
  write_new_field();
  context_->write(token_, value);
}

auto json_writer_array::write_value_quoted(std::string_view value) -> void {
  write_new_field();
  context_->write(token_, "\"");
  // TODO: Should escape the string as we write.
  context_->write(token_, detail::json_escape_string(value));
  context_->write(token_, "\"");
}

auto json_writer_array::close() -> void {
  if (context_) {
    context_->write(token_, " ]");
    context_->unregister_context(token_);
  }
  context_.reset();
}

}  // namespace ubench::sjson
