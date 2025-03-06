#include "str_intern.h"

auto intern_forward_list::intern(std::string_view str) -> const std::string& {
  for (auto& interned : strings_) {
    if (interned == str) {
      return interned;
    }
  }

  interned_++;
  return strings_.emplace_front(str);
};
