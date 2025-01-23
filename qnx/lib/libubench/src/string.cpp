#include "ubench/string.h"

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

}  // namespace ubench::string
