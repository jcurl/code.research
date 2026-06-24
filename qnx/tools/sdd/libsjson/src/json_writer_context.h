#ifndef SJSON_JSON_WRITER_CONTEXT_H
#define SJSON_JSON_WRITER_CONTEXT_H

#include <ostream>
#include <stack>
#include <string_view>

#include "sjson/json_writer.h"

namespace ubench::sjson::detail {

/// @brief Manages the context for the jason_writer.
class json_writer_context {
 public:
  /// @brief Constructor that takes the stream users can write to.
  ///
  /// @param ostream the stream writes can write to.
  json_writer_context(std::ostream& ostream) : ostream_{ostream} {}

  json_writer_context(const json_writer_context&) = delete;
  auto operator=(json_writer_context&) -> json_writer_context& = delete;
  json_writer_context(json_writer_context&&) noexcept = delete;
  auto operator=(json_writer_context&&) noexcept
      -> json_writer_context& = delete;
  ~json_writer_context() { close(); };

  /// @brief Sets the configuration for writing to the context.
  ///
  /// The write it explicit. It should be written before the first call to
  /// write(), for the output to be consistent.
  ///
  /// @param config The configuration to set.
  auto config() -> json_writer_config& { return config_; }

  auto config() const -> const json_writer_config& { return config_; }

  /// @brief Register for a context
  ///
  /// Registering for a context helps detect usage errors. Every class that
  /// wants to write can only do so with the context it receives. The context
  /// tracks which objects are allowed to write.
  ///
  /// @param token the context returned by register_context function.
  ///
  /// @return the context token. The value returned is opaque.
  ///
  /// @exception json_writer_error raised if the token is invalid.
  auto register_context(token_t token) -> token_t;

  /// @brief Unregister for a context
  ///
  /// Contexts are managed as a stack. Only the current context can be
  /// unregistered.
  ///
  /// @param token the context returned by register_context function.
  ///
  /// @exception json_writer_error raised if the token is invalid.
  auto unregister_context(token_t token) -> void;

  /// @brief Write the text for the current context.
  ///
  /// @param token the context returned by the register_context() function.
  ///
  /// @param text the text to write.
  ///
  /// @return if writing was successful. This method doesn't throw an exception.
  /// If writing fails, then false is returned. Note, failure may also be if the
  /// context has been closed unexpectedly.
  auto write(token_t token, std::string_view text) -> bool;

  /// @brief Close the context.
  ///
  /// Closes the context. Usually called by the module that initialised this
  /// class. Because other objects may have a reference, closing allows not
  /// using the object before the destructor is called.
  auto close() noexcept -> void { closed_ = true; }

  /// @brief Tests if a character is an escape character.
  ///
  /// @param escape_char the character to test for. As all escape characters are
  /// in the first code page of unicode (0x00-0x7F), a character is sufficient.
  /// Should a unicode character be out of this range, it is not an escape
  /// character.
  ///
  /// @return the character to escape with (e.g. `\` . return_value), or 0 if
  /// not an escape character.
  auto is_json_escape_char(char escape_char) -> char;

  /// @brief Produce a properly quoted string, assuming the input is valid UTF8.
  ///
  /// @param str the input string to quote
  ///
  /// @return the quoted output string that can be written to a stream.
  auto json_escape_string(std::string_view str) -> std::string;

 private:
  bool closed_{false};
  std::ostream& ostream_;
  std::stack<token_t> context_{};
  token_t next_context_{};
  json_writer_config config_{};
};

}  // namespace ubench::sjson::detail

#endif
