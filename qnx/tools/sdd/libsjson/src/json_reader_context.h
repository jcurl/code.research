#ifndef SJSON_JSON_READER_CONTEXT_H
#define SJSON_JSON_READER_CONTEXT_H

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <string_view>

#include "sjson/json_reader.h"

namespace ubench::sjson::detail {

/// @brief The token found when parsing a JSON string.
enum class json_string_keyword : std::uint8_t {
  array_open,    //< The array '[' character.
  array_close,   //< The array ']' character.
  object_open,   //< The object '{' character.
  object_close,  //< The object '}' character.
  integer,       //< A signed integer
  string,        //< A string.
  bool_true,     //< 'true' keyword.
  bool_false,    //< 'false' keyword.
  null_value,    //< 'null' keyword.
  object_colon,  //< The object ':' character.
  comma          //< Field separator ',' character.
};

/// @brief A token after parsing.
class json_reader_token {
 public:
  /// @brief Constructor.
  /// @param keyword The keyword type found.
  /// @param value The stringview in the input that maps to this keyword.
  json_reader_token(
      json_string_keyword keyword, std::string_view value) noexcept
      : keyword_{keyword}, value_{value} {}

  /// @brief Get the keyword for this token.
  /// @return The keyword for this token.
  auto keyword() const noexcept -> json_string_keyword { return keyword_; }

  /// @brief Get the value for this token.
  /// @return The value for this token.
  auto value() const noexcept -> std::string_view { return value_; }

 private:
  json_string_keyword keyword_;
  std::string_view value_;
};

/// @brief Either a parse element from an array or an object.
using json_member = std::variant<json_key, json_token>;

/// @brief An open block on the context stack.
struct json_stack_entry {
  json_string_keyword keyword;  //< The opening keyword of the block.
  bool owned;                   //< true if an iterator is walking the block.
};

/// @brief The JSON Reader context between iterators.
class json_reader_context
    : public std::enable_shared_from_this<json_reader_context> {
 public:
  /// @brief Constructor.
  /// @param json A string_view of the input.
  json_reader_context(std::string_view json) noexcept : json_{json} {}

  /// @brief Copy the parse state.
  ///
  /// The copy reads the same input from the same position, with the same block
  /// stack, and can be advanced independently of the original.
  json_reader_context(const json_reader_context&) = default;
  auto operator=(const json_reader_context&) -> json_reader_context& = default;
  json_reader_context(json_reader_context&&) noexcept = default;
  auto operator=(json_reader_context&&) noexcept
      -> json_reader_context& = default;
  ~json_reader_context() noexcept { close(); };

  /// @brief Parse the context string for the next keyword.
  ///
  /// After parsing, has_error() may not be set, and position() is updated.
  ///
  /// @return A keyword found.
  auto get_keyword() -> std::optional<json_reader_token>;

  /// @brief Parse the context string for the token.
  /// @return The next token. The value std::nullopt is returned when: (a) there
  /// is a parsing error, so check if there was an error; (b) end of an array or
  /// object block, so an iterator can exit; (c) end of the file.
  auto get_token() -> std::optional<json_member>;

  /// @brief Skip the rest of the block at the top of the stack.
  ///
  /// Consumes every token up to and including the matching close of the
  /// current block, so the next get_token() returns the token after it. Does
  /// nothing at depth zero. If the input ends before the block is closed, an
  /// error is recorded. Check if there was an error parsing by observing the
  /// has_error() method.
  auto skip_block() -> void;

  /// @brief Skip the contents of a container that an iterator never entered.
  ///
  /// The token is the current element of an iterator at the given depth. If it
  /// is a container that was read but not entered, and no iterator owns its
  /// contents, then the contents are skipped with skip_block().
  ///
  /// @param depth The depth of the iterator that holds the token.
  /// @param token The current element of that iterator.
  /// @return true if the contents were skipped.
  auto skip_container(std::size_t depth, const json_token& token) -> bool;

  /// @brief Close the context.
  ///
  /// Closes the context. Usually called by the module that initialised this
  /// class. Because other objects may have a reference, closing allows not
  /// using the object before the destructor is called.
  auto close() noexcept -> void { closed_ = true; }

  /// @brief Checks if this instance has reached the end of the file.
  /// @return true if the end of the file is reached.
  auto is_closed() const noexcept -> bool { return closed_; }

  /// @brief Check if a parsing error was found.
  /// @return true if the input contained an invalid sequence.
  auto has_error() const noexcept -> bool { return error_; }

  /// @brief Record that a parsing error was found.
  auto set_error() noexcept -> void { error_ = true; }

  /// @brief Get the current offset into the input.
  /// @return The number of characters consumed from the input.
  auto position() const noexcept -> std::size_t { return json_pos_; }

  /// @brief The depth level in the JSON parsing.
  /// @return The level, where zero is the root.
  auto depth() const noexcept -> std::size_t { return stack_.size(); }

  /// @brief Set whether an iterator is walking the block on top of the stack.
  ///
  /// Getting the json_object or json_array is not enough, it must first iterate
  /// with begin() and end(), which is recorded here.
  auto set_owned(bool owned) noexcept -> void {
    if (!stack_.empty()) stack_.top().owned = owned;
  }

  /// @brief Check if an iterator is walking the block on top of the stack.
  auto is_owned() const noexcept -> bool {
    return !stack_.empty() && stack_.top().owned;
  }

 private:
  auto is_keyword_container(json_string_keyword keyword) const noexcept
      -> bool {
    return keyword == json_string_keyword::array_open ||
           keyword == json_string_keyword::object_open;
  }

  auto is_keyword_value(json_string_keyword keyword) const noexcept -> bool {
    return keyword == json_string_keyword::bool_false ||
           keyword == json_string_keyword::bool_true ||
           keyword == json_string_keyword::null_value ||
           keyword == json_string_keyword::integer ||
           keyword == json_string_keyword::string;
  }

  auto close_root() -> void;

  auto is_object() const noexcept -> bool {
    return stack_.top().keyword == json_string_keyword::object_open;
  }

  auto is_array() const noexcept -> bool {
    return stack_.top().keyword == json_string_keyword::array_open;
  }

  auto stack_enter(json_string_keyword keyword) -> void {
    assert(is_keyword_container(keyword));
    stack_.push(json_stack_entry{keyword, false});
  }

  auto stack_leave([[maybe_unused]] json_string_keyword keyword) noexcept
      -> void {
    assert((keyword == json_string_keyword::array_close && is_array()) ||
           (keyword == json_string_keyword::object_close && is_object()));
    stack_.pop();
  }

  std::string_view json_;
  std::size_t json_pos_{};
  std::stack<json_stack_entry> stack_{};
  bool error_{};
  bool closed_{};
  bool state_start_block_{};
};

auto to_token(json_reader_token& token) -> std::optional<json_token>;

auto to_token(json_reader_token& token,
    std::shared_ptr<json_reader_context> context) -> std::optional<json_token>;

/// @brief Rebuilds each alternative of a token on a new context.
///
/// Containers are rebuilt on the context given here, so the copy never reads
/// through the source's context. Strings use the copy constructor, so a view
/// stays a view and an owned string is deep-copied.
class token_copier {
 public:
  /// @param context The context a copied container reads through.
  explicit token_copier(std::shared_ptr<json_reader_context> context)
      : context_{std::move(context)} {}

  auto operator()(const json_object&) const -> json_token {
    return json_object{context_};
  }

  auto operator()(const json_array&) const -> json_token {
    return json_array{context_};
  }

  auto operator()(const json_int& value) const -> json_token { return value; }

  auto operator()(const json_bool& value) const -> json_token { return value; }

  auto operator()(const json_string& value) const -> json_token {
    return json_string{value};
  }

  auto operator()(const json_null&) const -> json_token { return json_null{}; }

 private:
  std::shared_ptr<json_reader_context> context_;
};

/// @brief Makes a copy of a token and rebinds the new context.
///
/// The copy is built alternative by alternative, so the json_token variant's
/// copy constructor is never used.
/// @param value The token to copy.
/// @param context The context the copy reads through, if it is a container.
/// @return The copied token, bound to context.
inline auto copy_token(const json_token& value,
    std::shared_ptr<json_reader_context> context) -> json_token {
  return std::visit(token_copier{std::move(context)}, value);
}

}  // namespace ubench::sjson::detail

#endif
