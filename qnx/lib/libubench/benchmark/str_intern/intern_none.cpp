#include "str_intern.h"

namespace {

const std::string tmp_;

}

auto intern_none::intern([[maybe_unused]] std::string_view str)
    -> const std::string& {
  return tmp_;
};
