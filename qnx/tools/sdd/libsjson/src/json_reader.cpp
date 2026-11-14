#include "sjson/json_reader.h"

#include <cassert>

#include "json_reader_context.h"

namespace ubench::sjson {

json_reader::json_reader(std::string_view json) {
  context_ = std::make_shared<detail::json_reader_context>(json);
}

auto json_reader::root() const -> const std::optional<json_token> {
  // A moved-from reader has no input, so it has no root.
  if (!context_) return std::nullopt;

  if (context_->position() != 0)
    throw json_reader_error{"root() called after parsing input"};

  auto t = context_->get_token();
  if (!t) {
    // Empty input has no root. Invalid input has already set the error flag.
    return std::nullopt;
  }
  // Outside a container, get_token() can only return a token, never a key.
  assert(std::holds_alternative<json_token>(*t));
  return std::get<json_token>(std::move(*t));
}

auto json_reader::has_error() const noexcept -> bool {
  return context_ ? context_->has_error() : false;
}

auto json_reader::close() noexcept -> void {
  // TODO: Nothing for now. Unmap if required.
}

auto json_reader::is_closed() const noexcept -> bool {
  return context_ ? context_->is_closed() : true;
}

}  // namespace ubench::sjson
