#include <cassert>

#include "sjson/json_reader.h"
#include "json_reader_context.h"

namespace ubench::sjson {

json_object_iterator::json_object_iterator(
    std::shared_ptr<detail::json_reader_context> context)
    : context_{std::move(context)},
      json_pos_{context_ ? context_->position() : 0},
      depth_{context_ ? context_->depth() : 0} {
  if (context_ && context_->depth() == depth_) context_->set_owned(true);
  advance_token();
}

json_object_iterator::json_object_iterator(const json_object_iterator& other)
    : context_{other.context_ ? std::make_shared<detail::json_reader_context>(
                                    *other.context_)
                              : nullptr},
      json_pos_{other.json_pos_},
      depth_{other.depth_} {
  if (!other.current_) return;
  current_.emplace(json_string{other.current_->key_},
      detail::copy_token(other.current_->token_, context_));
}

auto json_object_iterator::operator=(const json_object_iterator& other)
    -> json_object_iterator& {
  if (this != &other) {
    *this = json_object_iterator{other};
  }
  return *this;
}

/// @brief Validate state of this object.
///
/// Must be called only if the context is defined. The stack is validated. What
/// this means is that it is expected that the location in the JSON file while
/// parsing matches the same depth as when this object was created. It would be
/// incorrect if this object is called while another iterator is active.
///
/// @return true if the object is in a valid state. If the object is invalid,
/// then false is returned, and the object is released internally.
/// @exception json_reader_error if there is a programmatic error. An invalid
/// JSON doesn't throw this error.
auto json_object_iterator::validate() -> bool {
  // It shouldn't be possible for the depth to be correct in a different block,
  // because our context is cleared when we reach the end of this block. A copy
  // has its own context, so it never shares this block's offset.

  if (!context_) return false;

  if (context_->has_error()) {
    release(false);
    return false;
  }

  // The current element is a container that was read but never entered, so its
  // contents are still unconsumed. Skip them before checking the depth. If an
  // iterator is open for that container, it owns the contents, so skip nothing
  // and let the depth check below report the mismatch.
  //
  // It happens when the iterator is advanced, without the container that it
  // returned itself being iterated. The json_xxx.begin() is never called, so it
  // never gets to call its destructor which would skip the block.
  if (current_ && context_->skip_container(depth_, current_->value())) {
    if (context_->has_error()) {
      release(false);
      return false;
    }
  }

  if (context_->depth() != depth_) {
    std::string msg{"json_object_iterator unexpected stack: "};
    msg += std::to_string(context_->depth()) + "; expected " +
           std::to_string(depth_);
    release(false);
    throw json_reader_error{std::move(msg)};
  }

  return true;
}

/// @brief Moves to the next token, or to the end state if there is none.
auto json_object_iterator::advance_token() -> void {
  if (!validate()) return;

  auto t = context_->get_token();
  if (!t || !std::holds_alternative<json_key>(*t)) {
    release(false);
    return;
  }
  current_ = std::get<json_key>(std::move(*t));
  json_pos_ = context_->position();
}

/// @brief Release resources for this object and mark it at the end of the
/// block.
///
/// @param inside_block true if the rest of the block is still unread, so it is
/// skipped first. Nothing is skipped if the stack doesn't match this iterator,
/// because another iterator owns that part of the input.
auto json_object_iterator::release(bool inside_block) -> void {
  if (!context_) return;

  if (inside_block) {
    if (current_) context_->skip_container(depth_, current_->value());
    if (context_->depth() == depth_) context_->skip_block();
  }

  if (context_->depth() == depth_) context_->set_owned(false);
  current_.reset();
  context_.reset();
  json_pos_ = 0;
  depth_ = 0;
}

auto json_object_iterator::operator++() -> json_object_iterator& {
  if (context_) advance_token();
  return *this;
}

auto json_object_iterator::operator++(int) -> json_object_iterator {
  // Costly: the copy has its own context, and destroying it skips the rest of
  // its block. Each it++ therefore walks the remaining keys, so a loop that
  // uses it++ is O(n^2). Use ++it where the old value isn't needed.
  json_object_iterator previous = *this;
  if (context_) advance_token();
  return previous;
}

}  // namespace ubench::sjson
