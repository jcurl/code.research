#include "sjson/json_writer.h"

#include <fstream>
#include <memory>

#include "json_writer_context.h"

namespace ubench::sjson {

json_writer::json_writer(std::string file_name)
    : ostream_(std::make_unique<std::ofstream>(std::move(file_name))) {
  if (!ostream_ || !ostream_->good()) {
    throw json_writer_error("Failed to open output file");
  }

  // The context_ (a shared pointer) may live longer than the ostream_. Ensure
  // that the context_.close() is called before the ostream_ is freed.
  context_ = std::make_shared<detail::json_writer_context>(*ostream_);
  token_ = context_->register_context(detail::token_t{});
}

json_writer::json_writer(std::string_view file_name)
    : ostream_(std::make_unique<std::ofstream>(std::string(file_name))) {
  if (!ostream_ || !ostream_->good()) {
    throw json_writer_error("Failed to open output file");
  }

  // The context_ (a shared pointer) may live longer than the ostream_. Ensure
  // that the context_.close() is called before the ostream_ is freed.
  context_ = std::make_shared<detail::json_writer_context>(*ostream_);
  token_ = context_->register_context(detail::token_t{});
}

json_writer::json_writer(std::filesystem::path file_name)
    : ostream_(std::make_unique<std::ofstream>(std::move(file_name))) {
  if (!ostream_ || !ostream_->good()) {
    throw json_writer_error("Failed to open output file");
  }

  // The context_ (a shared pointer) may live longer than the ostream_. Ensure
  // that the context_.close() is called before the ostream_ is freed.
  context_ = std::make_shared<detail::json_writer_context>(*ostream_);
  token_ = context_->register_context(detail::token_t{});
}

json_writer::json_writer(std::ostream& ostream) {
  // The context_ may live longer than this object, but is protected by the
  // context_.close() method, which will prevent usage avoiding undefined
  // behaviour. Therefore, the ostream only needs to outlive this object to be
  // well-defined.
  context_ = std::make_shared<detail::json_writer_context>(ostream);
  token_ = context_->register_context(detail::token_t{});
}

auto json_writer::write_object() -> json_writer_object {
  if (root_populated_)
    throw json_writer_error("Only a single root element supported");
  root_populated_ = true;
  detail::token_t new_token = context_->register_context(token_);
  return {new_token, context_};
}

auto json_writer::write_array() -> json_writer_array {
  if (root_populated_)
    throw json_writer_error("Only a single root element supported");
  root_populated_ = true;
  detail::token_t new_token = context_->register_context(token_);
  return {new_token, context_};
}

auto json_writer::close() -> void {
  // Ensure the context knows we're done. It will block usage to the ostream
  // reference it has.
  context_->close();

  // We're closed, we don't unregister the context. We assume the context might
  // be closed after an exception (so it might be corrupt), and we don't need to
  // use the context after this anyway.
}

}  // namespace ubench::sjson
