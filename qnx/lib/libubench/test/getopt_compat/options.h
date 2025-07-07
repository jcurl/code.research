#ifndef UBENCH_OPTIONS_COMPAT_H
#define UBENCH_OPTIONS_COMPAT_H

#include <optional>
#include <string>
#include <vector>

namespace ubench::experimental {

/// @brief A wrapper around get_opt() from the standard C-library.
///
/// All methods in this class are not thread-safe. They use the same
/// implementation as getopt() which relies on global variables defined in the
/// C-library. Do not use two instances of this class, even with different
/// options.
class options {
 public:
  /// @brief Construct the options from arguments given to your main() function.
  ///
  /// @param argc the number of arguments given to your application. Argument 0
  /// is the name of the program.
  ///
  /// @param argv a vector of the arguments of size argc.
  ///
  /// @param options a string defining the options compatible with getopt().
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
  explicit options(int argc, const char* const argv[], const char* options);

  /// @brief Construct the options from a vector of arguments.
  ///
  /// @param prog_name the name of the program, typically the first argument
  /// provided.
  ///
  /// @param args a vector of arguments copied from the parameter set of main,
  /// into the vector.
  ///
  /// @param options a string defining the options compatible with getopt().
  explicit options(std::string prog_name, std::vector<std::string> args,
      std::string options);

  options(const options& other) = delete;
  auto operator=(const options& other) -> options& = delete;
  options(options&& other) = default;
  auto operator=(options&& other) -> options& = default;
  ~options() = default;

  /// @brief Reset parsing the arguments to the beginning
  auto reset() -> void;

  /// @brief Iterate repeatedly to parse the options given in the constructor.
  ///
  /// @return the argument that was parsed (the character), or -1 if there are
  /// no more arguments, in which case parsing can stop.
  [[nodiscard]] auto getopt() -> int;

  /// @brief Get the value of the argument at the current position.
  ///
  /// This should only be called after getting an argument from getopt() that
  /// expects a value.
  ///
  /// @return If there is an argument, return a copy of the value for the
  /// argument.
  [[nodiscard]] auto optarg() -> std::optional<std::string>;

  /// @brief Returns the erroneous option.
  ///
  /// @return Return the option that caused an error. This is given in case of
  /// getopt() returning '?'.
  [[nodiscard]] auto optopt() -> int;

  /// @brief The name of the program, as given in the constructor.
  ///
  /// @return the name of the program, as given in the constructor.
  [[nodiscard]] auto prog_name() -> const std::string&;

  [[nodiscard]] auto args() const -> const std::vector<std::string>& {
    return args_;
  }

  [[nodiscard]] auto argsreal() const -> const std::vector<char*>& {
    return getopt_args_;
  }

  [[nodiscard]] auto optstring() const -> const std::string& {
    return options_;
  }

 private:
  std::string prog_name_{};
  std::vector<std::string> args_{};
  std::string options_{};

  std::vector<char*> getopt_args_{};
  std::string optarg_{};
};

}  // namespace ubench::experimental

#endif