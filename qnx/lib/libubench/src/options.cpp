#include "ubench/options.h"

#include <array>
#include <cctype>
#include <iostream>
#include <iterator>
#include <stdexcept>

#include "stdext/expected.h"

namespace ubench {

namespace {

enum class option_arg {
  not_defined,   ///< Argument not specified.
  single_opt,    ///< No argument expected.
  arg_required,  ///< Argument expected.
};

auto is_valid_opt(const char optarg) -> bool {
  if (optarg == '?') return true;
  return std::isalnum(optarg) != 0;
}

auto build_option_map(std::string_view options) -> std::array<option_arg, 128> {
  std::array<option_arg, 128> option_map{};

  char optchar = 0;
  std::size_t options_pos = 0;
  while ((optchar = options[options_pos]) != 0) {
    if (is_valid_opt(optchar)) {
      char optchar_next = options_pos + 1 < options.length()
                              ? options[options_pos + 1]
                              : (char)0;
      if (optchar_next == 0 || optchar_next != ':') {
        // is_valid_opt() ensures that optchar is in the range of the array.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        option_map[optchar] = option_arg::single_opt;
      } else {
        // is_valid_opt() ensures that optchar is in the range of the array.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        option_map[optchar] = option_arg::arg_required;
        options_pos++;
      }
    } else {
      throw std::invalid_argument("Invalid option in options string");
    }
    options_pos++;
  }
  return option_map;
}

auto inline str_char(std::string_view opt, std::size_t index) -> char {
  return index >= opt.length() ? (char)0 : opt[index];
}

}  // namespace

auto options::parse_options(const std::vector<std::string_view>& args,
    std::string_view options) -> void {
  auto option_map = build_option_map(options);

  bool parse_options = true;
  for (auto arg_pos = args.begin(); parse_options && arg_pos != args.end();
       arg_pos = std::next(arg_pos)) {
    const auto& arg = *arg_pos;

    if (arg.length() > 0 && arg[0] == '-') {
      if (arg.length() == 1) {
        // Stop parsing. This argument forms part of the data given to the user.
        parse_options = false;
        user_args_ = std::vector<std::string_view>(arg_pos, args.end());
      } else if (arg.length() == 2 && arg[1] == '-') {
        // Stop parsing. This is a double dash. move to the next argument.
        parse_options = false;
        user_args_ =
            std::vector<std::string_view>(std::next(arg_pos), args.end());
      } else {
        std::size_t char_pos = 1;
        bool parse_arg = true;
        char optchar = 0;
        while (parse_arg && (optchar = str_char(arg, char_pos)) != 0) {
          if (is_valid_opt(optchar)) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            switch (option_map[optchar]) {
              case option_arg::not_defined: {
                // Not in the options string.
                auto err = option_error{optchar, option_error_kind::unexpected};
                args_.emplace_back(
                    stdext::unexpected<option_type::error_type>(err));
                break;
              }
              case option_arg::single_opt: {
                args_.emplace_back(optchar);
                break;
              }
              case option_arg::arg_required: {
                if (char_pos + 1 < arg.length()) {
                  args_.emplace_back(option(optchar, arg.substr(char_pos + 1)));
                  parse_arg = false;
                } else {
                  if (std::next(arg_pos) == args.end()) {
                    auto err =
                        option_error{optchar, option_error_kind::missing};
                    args_.emplace_back(
                        stdext::unexpected<option_type::error_type>(err));
                    parse_arg = false;
                  } else {
                    arg_pos = std::next(arg_pos);
                    args_.emplace_back(option(optchar, *arg_pos));
                  }
                }
                break;
              }
            }
          } else {
            // Invalid option.
            auto err = option_error{optchar, option_error_kind::unexpected};
            args_.emplace_back(
                stdext::unexpected<option_type::error_type>(err));
          }
          char_pos++;
        }
      }
    } else {
      parse_options = false;
      user_args_ = std::vector<std::string_view>(arg_pos, args.end());
    }
  }
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
options::options(int argc, const char* const argv[], const char* options)
    : options_{options} {
  if (argc == 0) return;

  prog_name_ =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      argv[0] != nullptr ? std::string_view{argv[0]} : std::string_view{};
  if (argc == 1) return;

  // We have to sanitise every argument as being possibly `nullptr`.
  std::vector<std::string_view> args{};
  for (int argp = 1; argp < argc; argp++) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::string_view arg = argv[argp] != nullptr ? std::string_view{argv[argp]}
                                                 : std::string_view{};
    args.emplace_back(arg);
  }
  parse_options(args, options_);
}

options::options(
    std::string prog_name, std::vector<std::string> argv, std::string options)
    : options_{std::move(options)},
      local_prog_name_{std::move(prog_name)},
      local_args_{std::move(argv)} {
  prog_name_ = local_prog_name_;
  std::vector<std::string_view> args = {local_args_.begin(), local_args_.end()};
  parse_options(args, options_);
}

options::options(std::string_view prog_name, std::vector<std::string_view> argv,
    std::string options)
    : prog_name_{prog_name}, options_{std::move(options)} {
  parse_options(argv, options_);
}

options::options(const options& other) {
  if (other.local_prog_name_.empty()) {
    // We don't own the prog_name_, so just copy the view.
    prog_name_ = other.prog_name_;
  } else {
    local_prog_name_ = other.local_prog_name_;
    prog_name_ = std::string_view(local_prog_name_);
  }

  options_ = other.options_;
  if (other.local_args_.empty()) {
    // We don't own the prog_name, so just copy the view.
    args_ = other.args_;
    user_args_ = other.user_args_;
  } else {
    // Because we have to copy the complete structure, we copy the args, and
    // have to parse them again. We can't just update the args, because they are
    // views within local_args_.
    local_args_ = other.local_args_;
    std::vector<std::string_view> args = {
        local_args_.begin(), local_args_.end()};
    parse_options(args, options_);
  }
}

auto options::print_error(option_error err) -> void {
  switch (err.kind()) {
    case ubench::option_error_kind::missing:
      std::cerr << "Error: Option -" << err.get_option()
                << " requires an operand" << std::endl;
      break;
    case ubench::option_error_kind::unexpected:
      std::cerr << "Error: Unknown option -" << err.get_option() << std::endl;
  }
}

}  // namespace ubench
