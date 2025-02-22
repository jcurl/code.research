#include <iostream>
#include <string>

#include "mallopt.h"

namespace impl {

auto parse_mallopt_arg([[maybe_unused]] std::string_view mallopt_arg)
    -> std::vector<std::tuple<int, int, std::string>> {
  return {};
}

auto print_mallopt_help() -> void { std::cout << std::endl; }

}  // namespace impl
