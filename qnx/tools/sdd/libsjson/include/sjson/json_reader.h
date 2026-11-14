#ifndef SJSON_JSON_READER_H
#define SJSON_JSON_READER_H

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace ubench::sjson {

namespace detail {

class json_reader_context;

}  // namespace detail

/// @brief Exception raised when the writer has detected a programming error.
class json_reader_error final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// @brief A 64-bit integer from the JSON file.
class json_int {
 public:
  /// @brief Initialise the json_int with the value specified.
  /// @param value The value to associted wit hthis object.
  constexpr json_int(std::int64_t value) noexcept : value_{value} {}

  /// @brief Get the value of this object.
  constexpr operator std::int64_t() const noexcept { return value_; }

 private:
  std::int64_t value_;
};

/// @brief A boolean from the JSON file.
class json_bool {
 public:
  /// @brief Initialise the json_bool with the value specified.
  /// @param value The value to initialise with/
  constexpr json_bool(bool value) noexcept : value_{value} {}

  /// @brief Get the value of this object.
  constexpr operator bool() const noexcept { return value_; }

 private:
  bool value_;
};

/// @brief A JSON null object.
///
/// There is no value associated with this object.
class json_null {
 public:
  /// @brief Instantiate a JSON token that represents a null object.
  constexpr json_null() noexcept {}
};

/// @brief A string from the JSON file.
///
/// Maintains a reference to the original JSON snippet if possible, or a
/// converted string that is unescaped. This prevents having to allocate longer
/// strings if we don't need to.
class json_string {
 public:
  /// @brief Initialise an empty string.
  json_string() noexcept = default;

  /// @brief Initialise the json_string with the string_view value.
  /// @param value The string_view to initialise with.
  json_string(std::string_view value) noexcept : data_{value} {}

  /// @brief Initialise the json_string with the string.
  /// @param value The string to take over
  json_string(std::string value) noexcept : data_{std::move(value)} {}

  /// @brief Initialise the json_string with a C string.
  ///
  /// Provided so that a string literal (e.g. json_string{"xx"}) does not
  /// select between the string_view and string overloads ambiguously.
  /// @param value The C string to initialise with.
  json_string(const char* value) noexcept : data_{std::string_view{value}} {}

  /// @brief Copy constructor.
  ///
  /// If the other object was instantiated with a string view, this is very
  /// cheap. If it was instantiated with a string, that string is copied.
  ///
  /// @param other The other object to make a copy of.
  json_string(const json_string& other) = default;

  /// @brief Move constructor.
  /// @param other The other object to move into this one. The other object is
  /// undefined after this operation.
  json_string(json_string&& other) noexcept = default;

  /// @brief Copy assignment operator.
  ///
  /// If the other object was instantiated with a string view, this is very
  /// cheap. If it was instantiated with a string, that string is copied.
  ///
  /// @param other The other object to copy from to this object.
  /// @return The reference to this object.
  auto operator=(const json_string& other) -> json_string& {
    if (this != &other) data_ = other.data_;
    return *this;
  }

  /// @brief Move assignment operator.
  /// @param other The other object to move into this one. The other object is
  /// undefined after this operation.
  /// @return The reference to this object.
  auto operator=(json_string&& other) noexcept -> json_string& {
    if (this != &other) data_ = std::move(other.data_);
    return *this;
  }

  /// @brief Get a string_view reference to this value implicitly.
  operator std::string_view() const noexcept {
    return std::holds_alternative<std::string_view>(data_)
               ? std::get<std::string_view>(data_)
               : std::string_view{std::get<std::string>(data_)};
  }

  /// @brief Get the number of characters in the string.
  /// @return The size, identical to that of the string_view.
  auto size() const noexcept -> std::size_t {
    return std::string_view{*this}.size();
  }

  /// @brief Get the number of characters in the string.
  /// @return The length, identical to that of the string_view.
  auto length() const noexcept -> std::size_t {
    return std::string_view{*this}.length();
  }

  /// @brief Check whether the string is empty.
  /// @return true if the string has no characters, identical to the
  /// string_view.
  auto empty() const noexcept -> bool {
    return std::string_view{*this}.empty();
  }

  /// @brief Check whether the string refers to the original JSON snippet.
  /// @return true if the value is a string_view, false if it owns its text.
  auto is_view() const noexcept -> bool {
    return std::holds_alternative<std::string_view>(data_);
  }

 private:
  std::variant<std::string, std::string_view> data_;
};

// Forward declarations. json_key holds a value of json_token, json_object has a
// pointer to json_key. Anything by-value must be a complete type. So these must
// be forward declared.
class json_key;
class json_array;
class json_array_iterator;
class json_object;
class json_object_iterator;

// Naming incomplete alternatives is fine; the variant is only instantiated
// after json_array, json_object and json_key are defined.

/// @brief A parsed JSON token.
using json_token = std::variant<json_object, json_array, json_int, json_bool,
    json_string, json_null>;

/// @brief A JSON array block, which provides iterators over its tokens.
class json_array {
 public:
  /// @brief Create a JSON array block.
  /// @param context The context. Because this is not defined, user code cannot
  /// create this object. This object is instantiated from json_reader, or while
  /// iterating.
  json_array(std::shared_ptr<detail::json_reader_context> context) noexcept
      : context_{std::move(context)} {}

  /// @brief Get an iterator to the first token of the array.
  /// @return An array iterator.
  auto begin() const -> json_array_iterator;

  /// @brief Get an iterator past the last token of the array.
  /// @return An array iterator indicating the end of parsing.
  auto end() const -> json_array_iterator;

 private:
  friend class json_array_iterator;
  friend class json_object_iterator;

  std::shared_ptr<detail::json_reader_context> context_;
};

/// @brief A JSON object block, which provides iterators over its keys.
class json_object {
 public:
  /// @brief Create a JSON object block.
  /// @param context The context. Because this is not defined, user code cannot
  /// create this object. This object is instantiated from json_reader, or while
  /// iterating.
  json_object(std::shared_ptr<detail::json_reader_context> context) noexcept
      : context_{std::move(context)} {}

  /// @brief Get an iterator to the first key of the object.
  /// @return An object iterator.
  auto begin() const -> json_object_iterator;

  /// @brief Get an iterator past the last key of the object.
  /// @return An object iterator indicating the end of parsing.
  auto end() const -> json_object_iterator;

 private:
  friend class json_array_iterator;
  friend class json_object_iterator;

  std::shared_ptr<detail::json_reader_context> context_;
};

/// @brief A JSON key as part of an object.
///
/// The JSON key is a member of an object, where there is the key and the value
/// is a new json_token.
class json_key {
 public:
  /// @brief Initialise the JSON key.
  /// @param key The name of the key in the JSON object.
  /// @param token The token for this key.
  json_key(json_string key, json_token token) noexcept
      : key_{std::move(key)}, token_{std::move(token)} {}

  json_key(const json_key&) = default;
  auto operator=(const json_key&) -> json_key& = default;
  json_key(json_key&&) noexcept = default;
  auto operator=(json_key&&) noexcept -> json_key& = default;

  /// @brief Get the key for this object.
  /// @return the key, unescaped.
  auto key() const noexcept -> std::string_view { return key_; }

  /// @brief Get the value for this object.
  /// @return The token.
  auto value() const noexcept -> const json_token& { return token_; }

 private:
  friend class json_object_iterator;

  json_string key_;
  json_token token_;
};

/// @brief An iterator over the tokens of a JSON array block.
class json_array_iterator {
 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = json_token;
  using difference_type = std::ptrdiff_t;
  using pointer = const json_token*;
  using reference = const json_token&;

  /// @brief Create an iterator at the end, which compares equal to end().
  json_array_iterator() noexcept = default;

  /// @brief Create an array iterator.
  /// @param context The context. Because this is not defined, user code cannot
  /// create this object. This object is instantiated from json_array.
  json_array_iterator(std::shared_ptr<detail::json_reader_context> context);

  /// @brief Copy the position. The copy has its own parse state over the same
  /// input, so it can be advanced independently of this iterator.
  /// @param other The other object.
  json_array_iterator(const json_array_iterator& other);

  /// @brief Copy the position. The copy has its own parse state over the same
  /// input, so it can be advanced independently of this iterator.
  /// @param other The other object.
  auto operator=(const json_array_iterator& other) -> json_array_iterator&;

  /// @brief Take over the position and parse state of another iterator. The
  /// other iterator is left at the end.
  /// @param other The other object.
  json_array_iterator(json_array_iterator&& other) noexcept
      : context_{std::exchange(other.context_, nullptr)},
        current_{std::move(other.current_)},
        json_pos_{std::exchange(other.json_pos_, 0)},
        depth_{std::exchange(other.depth_, 0)} {
    other.current_.reset();
  }

  /// @brief Finish the block of this iterator, then take over the position and
  /// parse state of another iterator. The other iterator is left at the end.
  /// @param other The other object.
  auto operator=(json_array_iterator&& other) -> json_array_iterator& {
    if (this != &other) {
      release(true);
      context_ = std::exchange(other.context_, nullptr);
      current_ = std::move(other.current_);
      other.current_.reset();
      json_pos_ = std::exchange(other.json_pos_, 0);
      depth_ = std::exchange(other.depth_, 0);
    }
    return *this;
  }

  ~json_array_iterator() { release(true); }

  reference operator*() const;
  pointer operator->() const;

  json_array_iterator& operator++();

  json_array_iterator operator++(int);

  auto operator==(const json_array_iterator& other) const noexcept -> bool {
    return json_pos_ == other.json_pos_;
  }

  auto operator!=(const json_array_iterator& other) const noexcept -> bool {
    return !(*this == other);
  }

 private:
  auto validate() -> bool;
  auto advance_token() -> void;
  auto release(bool inside_block) -> void;

  std::shared_ptr<detail::json_reader_context> context_;
  std::optional<json_token> current_;
  std::size_t json_pos_{};
  std::size_t depth_{};
};

/// @brief An iterator over the keys of a JSON object block.
class json_object_iterator {
 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = json_key;
  using difference_type = std::ptrdiff_t;
  using pointer = const json_key*;
  using reference = const json_key&;

  /// @brief Create an iterator at the end, which compares equal to end().
  json_object_iterator() noexcept = default;

  /// @brief Create an object iterator.
  /// @param context The context. Because this is not defined, user code cannot
  /// create this object. This object is instantiated from json_object.
  json_object_iterator(std::shared_ptr<detail::json_reader_context> context);

  /// @brief Copy the position. The copy has its own parse state over the same
  /// input, so it can be advanced independently of this iterator.
  /// @param other The other object.
  json_object_iterator(const json_object_iterator& other);

  /// @brief Copy the position. The copy has its own parse state over the same
  /// input, so it can be advanced independently of this iterator.
  /// @param other The other object.
  auto operator=(const json_object_iterator& other) -> json_object_iterator&;

  /// @brief Take over the position and parse state of another iterator. The
  /// other iterator is left at the end.
  /// @param other The other object.
  json_object_iterator(json_object_iterator&& other) noexcept
      : context_{std::exchange(other.context_, nullptr)},
        current_{std::move(other.current_)},
        json_pos_{std::exchange(other.json_pos_, 0)},
        depth_{std::exchange(other.depth_, 0)} {
    other.current_.reset();
  }

  /// @brief Finish the block of this iterator, then take over the position and
  /// parse state of another iterator. The other iterator is left at the end.
  /// @param other The other object.
  auto operator=(json_object_iterator&& other) -> json_object_iterator& {
    if (this != &other) {
      release(true);
      context_ = std::exchange(other.context_, nullptr);
      current_ = std::move(other.current_);
      other.current_.reset();
      json_pos_ = std::exchange(other.json_pos_, 0);
      depth_ = std::exchange(other.depth_, 0);
    }
    return *this;
  }

  ~json_object_iterator() { release(true); }

  reference operator*() const;
  pointer operator->() const;

  json_object_iterator& operator++();

  json_object_iterator operator++(int);

  auto operator==(const json_object_iterator& other) const noexcept -> bool {
    return json_pos_ == other.json_pos_;
  }

  auto operator!=(const json_object_iterator& other) const noexcept -> bool {
    return !(*this == other);
  }

 private:
  auto validate() -> bool;
  auto advance_token() -> void;
  auto release(bool inside_block) -> void;

  std::shared_ptr<detail::json_reader_context> context_;
  std::optional<json_key> current_;
  std::size_t json_pos_{};
  std::size_t depth_{};
};

// These inline operators must be after json_key and json_token is completely
// defined. Putting them in the class itself would ask the compiler to
// instantiate the value before its implementation and layout are defined..

inline auto json_array_iterator::operator*() const -> reference {
  return *current_;
}

inline auto json_array_iterator::operator->() const -> pointer {
  return current_ ? &*current_ : nullptr;
}

inline auto json_object_iterator::operator*() const -> reference {
  return *current_;
}

inline auto json_object_iterator::operator->() const -> pointer {
  return current_ ? &*current_ : nullptr;
}

inline auto json_array::begin() const -> json_array_iterator {
  return json_array_iterator{context_};
}

inline auto json_array::end() const -> json_array_iterator {
  // A null context marks the end. current_ stays empty, so iteration stops.
  return json_array_iterator{nullptr};
}

inline auto json_object::begin() const -> json_object_iterator {
  return json_object_iterator{context_};
}

inline auto json_object::end() const -> json_object_iterator {
  // A null context marks the end. current_ stays empty, so iteration stops.
  return json_object_iterator{nullptr};
}

/// @brief The root JSON reader object that is instantated by the user.
///
/// Instantiate this object, and then get iterate over the object that parses as
/// it is being iterated. This object must remain alive while iterating and
/// using any object that is returned by this object.
class json_reader {
 public:
  json_reader(const json_reader&) = delete;
  auto operator=(const json_reader&) -> json_reader& = delete;
  json_reader(json_reader&&) noexcept = default;
  auto operator=(json_reader&&) noexcept -> json_reader& = default;
  ~json_reader() { close(); }

  /// @brief Create a new instance to read the JSON snippet from the
  /// string_view.
  /// @param json The string_view to parse the JSON snippet.
  json_reader(std::string_view json);

  /// @brief Get the root element of the JSON document.
  /// @return The token if present, else the document has no root.
  /// @exception json_reader_error if the reader is not at the start of the
  /// input.
  auto root() const -> const std::optional<json_token>;

  /// @brief Check if a parsing error was found.
  /// @return true if the input contained an invalid sequence.
  auto has_error() const noexcept -> bool;

  /// @brief End the writer object
  auto close() noexcept -> void;

  /// @brief Checks if this instance has reached the end of the file.
  /// @return true if the end of the file is reached.
  auto is_closed() const noexcept -> bool;

 private:
  std::shared_ptr<detail::json_reader_context> context_{};
};

}  // namespace ubench::sjson

#endif
