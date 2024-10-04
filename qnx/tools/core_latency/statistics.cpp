#include "statistics.h"

auto statistics::insert(value_type value) -> void {
  this->set_.insert(value);
  this->sum_ += value;
}

auto statistics::clear() -> void {
  this->set_.clear();
  this->sum_ = 0;
}

auto statistics::min() const noexcept -> value_type {
  if (this->set_.empty()) return 0;
  return *(this->set_.cbegin());
}

auto statistics::max() const noexcept -> value_type {
  if (this->set_.empty()) return 0;
  return *(this->set_.crbegin());
}

auto statistics::size() const noexcept -> unsigned long {
  return this->set_.size();
}

auto statistics::median() const noexcept -> value_type {
  if (this->set_.empty()) return 0;

  auto len = this->set_.size();
  auto midIt = this->set_.cbegin();
  if (len % 2 == 1) {
    std::advance(midIt, len / 2);
    return *midIt;
  } else {
    std::advance(midIt, len / 2 - 1);
    return (*midIt + *std::next(midIt)) / 2;
  }
}

auto statistics::average() const noexcept -> unsigned int {
  if (this->set_.empty()) return 0;

  // static cast is fine, as the average is bounded in the range defined by
  // the input type.
  return static_cast<int>(this->sum_ / this->set_.size());
}
