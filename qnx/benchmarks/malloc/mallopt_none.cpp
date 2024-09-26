#include "mallopt.h"

#include <iostream>
#include <string_view>

auto mallopt_options::parse_mallopt_arg(std::string_view) -> bool {
  return false;
}

auto mallopt_options::print_mallopt_help() -> void { std::cout << std::endl; }