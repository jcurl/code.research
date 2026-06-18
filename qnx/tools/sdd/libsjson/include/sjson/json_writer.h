#ifndef SJSON_JSON_WRITER_H
#define SJSON_JSON_WRITER_H

#include <filesystem>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace ubench::sjson {

/// @brief Exception raised when the writer has detected a programming error.
class json_writer_error final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

namespace detail {

// Defined as an internal detail.
class json_writer_context;

/// @brief The type for our random token.
///
/// The token is an internal detail, not to be used outside of the
/// json_writer_context class. Its meaning is completely opaque to
/// the outside world.
class token_t {
 public:
  token_t() = default;
  token_t(const token_t&) = default;
  auto operator=(const token_t&) -> token_t& = default;
  token_t(token_t&&) noexcept = default;
  auto operator=(token_t&&) noexcept -> token_t& = default;
  ~token_t() = default;

  auto operator==(const token_t& o) const noexcept -> bool {
    return value == o.value;
  }

  auto operator!=(const token_t& o) const noexcept -> bool {
    return value != o.value;
  }

 private:
  friend class json_writer_context;
  auto operator++() noexcept -> token_t& {
    ++value;
    return *this;
  }

  auto operator++(int) noexcept -> token_t {
    token_t tmp = *this;
    ++value;
    return tmp;
  }

  std::uint16_t value{};
};

}  // namespace detail

// Forward declarations
class json_writer_object;
class json_writer_array;

/// @brief The root JSON writer object that is instantated by the user.
///
/// Instantiate this object, and then use the methods write_object() or
/// write_array() to create the first element. When the elements are finished,
/// either let the object be destroyed, or call close() explicitly.
class json_writer {
 public:
  json_writer(const json_writer&) = delete;
  auto operator=(const json_writer&) -> json_writer& = delete;
  json_writer(json_writer&&) noexcept = default;
  auto operator=(json_writer&&) noexcept -> json_writer& = default;
  ~json_writer() { close(); }

  /// @brief Create a new instance writing to file_name.
  ///
  /// @param file_name the file name to write to.
  json_writer(std::string file_name);

  /// @brief Create a new instance writing to file_name.
  ///
  /// @param file_name the file name to write to.
  json_writer(std::string_view file_name);

  /// @brief Create a new instance writing to file_name.
  ///
  /// @param file_name the file name to write to.
  json_writer(std::filesystem::path file_name);

  /// @brief Create a new instance writing to the stream.
  ///
  /// @param ostream the stream to write to. This object must have a lifetime
  /// that is longer than the json_writer object.
  json_writer(std::ostream& ostream);

  /// @brief Create a new object.
  ///
  /// @return the context to write elements in the object.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  [[nodiscard]] auto write_object() -> json_writer_object;

  /// @brief Create a new array.
  ///
  /// @return the context to write elements in the array.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  [[nodiscard]] auto write_array() -> json_writer_array;

  /// @brief Close the file
  ///
  /// Terminates the elements of all open objects, writes it to the file and
  /// closes the file.
  auto close() -> void;

 private:
  std::unique_ptr<std::ostream> ostream_;
  std::shared_ptr<detail::json_writer_context> context_{};
  detail::token_t token_{};
  bool root_populated_{false};
};

/// @brief The root JSON writer that can write objects.
class json_writer_object {
 public:
  json_writer_object(const json_writer_object&) = delete;
  auto operator=(const json_writer_object&) -> json_writer_object& = delete;
  json_writer_object(json_writer_object&&) noexcept = default;
  auto operator=(json_writer_object&&) noexcept
      -> json_writer_object& = default;
  ~json_writer_object() { close(); };

  /// @brief Write the start of a new object.
  ///
  /// @param key the key for the object as a string. Must be a valid UTF-8
  /// string.
  ///
  /// @return a new writer object.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  [[nodiscard]] auto write_object(std::string_view key) -> json_writer_object;

  /// @brief Write the start of a new array.
  ///
  /// @param key the key for the array as a string. Must be a valid UTF-8
  /// string.
  ///
  /// @return a new writer array.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  [[nodiscard]] auto write_array(std::string_view key) -> json_writer_array;

  /// @brief Write a string value.
  ///
  /// @param key the key for the value as a string. Must be a valid UTF-8
  /// string.
  ///
  /// @param value the value to write. Must be a valid UTF-8 string.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  auto write_value(std::string_view key, const char* value) -> void;

  /// @brief Write a string value.
  ///
  /// @param key the key for the value as a string. Must be a valid UTF-8
  /// string.
  ///
  /// @param value the value to write. Must be a valid UTF-8 string.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  auto write_value(std::string_view key, std::string_view value) -> void;

  /// @brief Write an integer value.
  ///
  /// @param key the key for the value as a string. Must be a valid UTF-8
  /// string.
  ///
  /// @param value the value to write.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  template <typename T,
      typename = std::void_t<decltype(std::to_string(std::declval<T>()))>>
  auto write_value(std::string_view key, T value) -> void {
    if constexpr (std::is_integral_v<T>) {
      write_value_unquoted(key, std::to_string(value));
    } else {
      write_value_quoted(key, std::to_string(value));
    }
  }

  /// @brief Write a boolean value.
  ///
  /// @param key the key for the value as a string. Must be a valid UTF-8
  /// string.
  ///
  /// @param value the value to write.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  auto write_value(std::string_view key, bool value) -> void;

  /// @brief Write a null value.
  ///
  /// @param key the key for the value as a string. Must be a valid UTF-8
  /// string.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  auto write_null(std::string_view key) -> void;

  /// @brief End the writer object
  ///
  /// @exception json_writer_error raised if used out of sequence. This doesn't
  /// occur if the object has already been closed.
  auto close() -> void;

 private:
  friend class json_writer;
  friend class json_writer_array;
  json_writer_object(
      detail::token_t t, std::shared_ptr<detail::json_writer_context> context);
  auto write_value_unquoted(std::string_view key, std::string value) -> void;
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  auto write_value_quoted(std::string_view key, std::string_view value) -> void;
  auto write_new_field(std::string_view key) -> void;

  detail::token_t token_;
  std::shared_ptr<detail::json_writer_context> context_{};
  bool has_element_{false};
};

/// @brief The root JSON writer that can write arrays.
class json_writer_array {
 public:
  json_writer_array(const json_writer_array&) = delete;
  auto operator=(const json_writer_array&) -> json_writer_array& = delete;
  json_writer_array(json_writer_array&&) noexcept = default;
  auto operator=(json_writer_array&&) noexcept -> json_writer_array& = default;
  ~json_writer_array() { close(); };

  /// @brief Write the start of a new object.
  ///
  /// @return a new writer object.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  [[nodiscard]] auto write_object() -> json_writer_object;

  /// @brief Write the start of a new array.
  ///
  /// @return a new writer array.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  [[nodiscard]] auto write_array() -> json_writer_array;

  /// @brief Write a string value.
  ///
  /// @param value the value to write. Must be a valid UTF-8 string.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  auto write_value(const char* value) -> void;

  /// @brief Write a string value.
  ///
  /// @param value the value to write. Must be a valid UTF-8 string.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  auto write_value(std::string_view value) -> void;

  /// @brief Write an integer value.
  ///
  /// @param value the value to write.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  template <typename T,
      typename = std::void_t<decltype(std::to_string(std::declval<T>()))>>
  auto write_value(T value) -> void {
    if constexpr (std::is_integral_v<T>) {
      write_value_unquoted(std::to_string(value));
    } else {
      write_value_quoted(std::to_string(value));
    }
  }

  /// @brief Write a boolean value.
  ///
  /// @param value the value to write.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  auto write_value(bool value) -> void;

  /// @brief Write a null value.
  ///
  /// @exception json_writer_error raised if used out of sequence.
  auto write_null() -> void;

  /// @brief End the writer object
  ///
  /// @exception json_writer_error raised if used out of sequence. This doesn't
  /// occur if the object has already been closed.
  auto close() -> void;

 private:
  friend class json_writer;
  friend class json_writer_object;
  json_writer_array(
      detail::token_t t, std::shared_ptr<detail::json_writer_context> context);
  auto write_value_unquoted(std::string value) -> void;
  auto write_value_quoted(std::string_view value) -> void;
  auto write_new_field() -> void;

  detail::token_t token_;
  std::shared_ptr<detail::json_writer_context> context_{};
  bool has_element_{false};
};

}  // namespace ubench::sjson

#endif
