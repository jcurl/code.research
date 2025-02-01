#include "ubench/string.h"

#include <cerrno>
#include <iostream>
#include <sstream>

namespace ubench::string {

auto split_args(std::string_view arg)
    -> std::optional<std::vector<std::string>> {
  std::vector<std::string> split_args{};

  std::string::size_type sz = 0;
  while (true) {
    std::string_view::size_type next = arg.find_first_of(',', sz);
    if (next == std::string_view::npos) {
      split_args.emplace_back(arg, sz, arg.size() - sz);
      return split_args;
    }

    split_args.emplace_back(arg, sz, next - sz);
    sz = next + 1;
  }
}

auto perror(int err) -> std::string {
  if (err == 0) return {};

  std::stringstream result{};
  result << strerror(err) << " (" << err << ")";
  return std::move(*result.rdbuf()).str();
}

auto perror(const std::string& msg) -> void {
  int e = errno;
  std::cerr << msg << " " << perror(e) << std::endl;
}

}  // namespace ubench::string
