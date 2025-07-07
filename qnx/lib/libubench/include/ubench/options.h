#ifndef UBENCH_OPTIONS_COMPAT_H
#define UBENCH_OPTIONS_COMPAT_H

#include <optional>
#include <string>
#include <vector>

#include "stdext/expected.h"

namespace ubench {

/// @brief Represents a valid, parsed option on the command line.
class option {
 public:
  /// @brief Provide an option that has no arguemnts.
  ///
  /// @param opt The option that was found on the command line.
  option(char opt) : option_{opt}, argument_{std::nullopt} {}

  /// @brief Provide an option where an argument is available.
  ///
  /// @param opt The option that was found on the command line.
  ///
  /// @param argument The argument that was associated with the option.
  option(char opt, std::string_view argument)
      : option_{opt}, argument_{argument} {}

  /// @brief Get the option character.
  ///
  /// @return the character representing the option.
  [[nodiscard]] auto get_option() const -> char { return option_; }

  /// @brief Get the argument with the option, if available.
  ///
  /// @return The optional argument.
  [[nodiscard]] auto argument() const
      -> const std::optional<std::string_view>& {
    return argument_;
  }

 private:
  char option_;
  std::optional<std::string_view> argument_;
};

/// @brief Specifies the kind of error found when parsing options.
enum class option_error_kind {
  unexpected,  ///< The option found is unexpected (not specified in the
               ///< optstring).
  missing,     ///< The option found is missing a required argument.
};

/// @brief Specifies an error when parsing options.
class option_error {
 public:
  /// @brief The option that has an unexpected option.
  ///
  /// @param opt The option character observed that has the error.
  option_error(char opt) : option_{opt}, kind_{option_error_kind::unexpected} {}

  /// @brief The option that has an error.
  ///
  /// @param opt The option character observed that has the error.
  ///
  /// @param kind The kind of error observed.
  option_error(char opt, option_error_kind kind) : option_{opt}, kind_{kind} {}

  /// @brief The option character found that is in error.
  ///
  /// @return The option character.
  [[nodiscard]] auto get_option() const -> char { return option_; }

  /// @brief The kind of error that was found.
  ///
  /// @return the kind of error that was found.
  [[nodiscard]] auto kind() const -> option_error_kind { return kind_; }

 private:
  char option_;
  option_error_kind kind_;
};

class options {
 public:
  using option_type = stdext::expected<option, option_error>;

  /// @brief Parse options from the main() function.
  ///
  /// The lifetime of this object is only valid while the argument block is
  /// valid (and not changed). A copy of the arguments are not made. If you need
  /// a copy of the arguments, then use the constructor that takes the arguments
  /// by value in the vector argv.
  ///
  /// @param argc The number of arguments, as given by main(), including the
  /// name of the program itself in arg[0].
  ///
  /// @param argv The vector of arguments, as given by main(), of length argc.
  ///
  /// @param options The option string to use when parsing the arguments.
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
  explicit options(int argc, const char* const argv[], const char* options);

  /// @brief Parse options that have already been placed into a vector.
  ///
  /// This version of the constructor maintains the lifetime of the arguments
  /// given by making a copy of the prog_name, argv, and options.
  ///
  /// @param prog_name The name of the program, usually given in argv[0].
  ///
  /// @param argv A vector of the arguments, excluding the program name, usually
  /// from argv[1] until the last element.
  ///
  /// @param options The option string to use when parsing the arguments.
  explicit options(std::string prog_name, std::vector<std::string> argv,
      std::string options);

  /// @brief Parse options that have been placed into a vector.
  ///
  /// The lifetime of this object is only valid while the argument block which
  /// the string_view elements point to are valid and are not changed. While a
  /// copy of the string_view is made, the underlying data is not copied.
  ///
  /// @param prog_name The name of the program, usually given in argv[0].
  ///
  /// @param argv A vector of the arguments, excluding the program name, usually
  /// from argv[1] until the last element.
  ///
  /// @param options The option string to use when parsing the arguments.
  explicit options(std::string_view prog_name,
      std::vector<std::string_view> argv, std::string options);

  options(const options& other);
  auto operator=(const options& other) -> options& {
    options{other}.swap(*this);
    return *this;
  }

  options(options&& other) = default;
  auto operator=(options&& other) -> options& = default;
  ~options() = default;

  /// @brief Swap this instance with another instance.
  ///
  /// @param other The other object to swap with.
  auto swap(options& other) noexcept -> void {
    std::swap(prog_name_, other.prog_name_);
    std::swap(args_, other.args_);
    std::swap(user_args_, other.user_args_);
    std::swap(options_, other.options_);
    std::swap(local_prog_name_, other.local_prog_name_);
    std::swap(local_args_, other.local_args_);
  }

  /// @brief The name of the program, as given in the constructor.
  ///
  /// @return the name of the program, as given in the constructor.
  [[nodiscard]] auto prog_name() const -> std::string_view {
    return prog_name_;
  }

  /// @brief Returns an iterator for the arguments after all the options were
  /// parsedl
  ///
  /// @return A vector of arguments. The elements in the vector are only valid
  /// while the options object is in scope.
  [[nodiscard]] auto args() const -> const std::vector<std::string_view>& {
    return user_args_;
  }

  /// @brief Returns the start of an iterator for the parsed options.
  ///
  /// @return The first element when parsing the options of type option_type.
  [[nodiscard]] auto begin() const -> std::vector<option_type>::const_iterator {
    return args_.cbegin();
  }

  /// @brief Returns the final element of an iterator for the parsed options.
  ///
  /// @return The final element when parsing the options of type option_type.
  [[nodiscard]] auto end() const -> std::vector<option_type>::const_iterator {
    return args_.cend();
  }

  /// @brief Print appropriate error message to the console for the error.
  ///
  /// @param err The option and kind of error.
  static auto print_error(option_error err) -> void;

 private:
  std::string_view prog_name_{};
  std::vector<option_type> args_{};
  std::vector<std::string_view> user_args_{};
  std::string options_{};

  // Local copies only if we own the data.
  std::string local_prog_name_{};
  std::vector<std::string> local_args_{};

  auto parse_options(const std::vector<std::string_view>& args,
      std::string_view options) -> void;
};

}  // namespace ubench

#endif
