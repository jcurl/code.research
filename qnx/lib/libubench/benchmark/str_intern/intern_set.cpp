#include "str_intern.h"

auto intern_ordered_set::intern(std::string_view str) -> const std::string& {
  auto result = set_.emplace(str);
  return *result.first;
};

auto intern_unordered_set::intern(std::string_view str) -> const std::string& {
  auto result = set_.emplace(str);
  return *result.first;
};
