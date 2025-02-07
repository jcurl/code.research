#include "ubench/string.h"

#include <cerrno>
#include <iostream>
#include <sstream>

namespace ubench::string {

auto split_args(std::string_view arg, unsigned int fields)
    -> std::vector<std::string_view> {
  std::vector<std::string_view> split_args{};

  std::string::size_type sz = 0;
  while (true) {
    if (fields == 0 || split_args.size() < fields - 1) {
      std::string_view::size_type next = arg.find_first_of(',', sz);
      if (next == std::string_view::npos) {
        split_args.emplace_back(arg.data() + sz, arg.size() - sz);
        return split_args;
      }

      split_args.emplace_back(arg.data() + sz, next - sz);
      sz = next + 1;
    } else {
      split_args.emplace_back(arg.data() + sz, arg.size() - sz);
      return split_args;
    }
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
