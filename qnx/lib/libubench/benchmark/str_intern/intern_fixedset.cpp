#include <forward_list>
#include <stdexcept>
#include <vector>

#include "str_intern.h"

namespace {

struct string_hash_block {
  string_hash_block(std::size_t hash, std::string* interned)
      : hash{hash}, interned{interned} {}

  std::size_t hash;
  std::string* interned;
};

}  // namespace

class intern_fixed_set::interned {
 public:
  interned(std::size_t buckets) : hash_mask_(buckets - 1) {
    set_.resize(buckets);
  };

  [[nodiscard]] auto size() const -> unsigned int { return interned_; }

  auto intern(std::string_view str) -> const std::string& {
    std::size_t h = svh_(str);
    std::size_t i = h & hash_mask_;
    for (const auto& block : set_[i]) {
      if (block.hash == h) {
        if (str == *block.interned) {
          return *block.interned;
        }
      }
    }

    // Didn't find the hash, so we add it.
    interned_++;
    auto& interned = strings_.emplace_front(str);
    set_[i].emplace_front(h, &interned);
    return interned;
  }

 private:
  std::size_t interned_{};
  std::vector<std::forward_list<string_hash_block>> set_{};
  std::forward_list<std::string> strings_{};
  std::size_t hash_mask_{};
  std::hash<std::string_view> svh_{};
};

// When compared with an implementation that didn't use "pimpl", the performance
// was measured to be the same on an i9-13980HX.

// For the "pimpl" pattern (using a unique_ptr on a forward declarated class),
// need to ensure that there is no definition (inline) of the constructor in the
// header file, else a move of "udp_talker_bpf" will not compile.
intern_fixed_set::~intern_fixed_set() = default;

intern_fixed_set::intern_fixed_set(std::size_t buckets) {
  if (buckets & (buckets - 1)) {
    throw std::invalid_argument("buckets is not a power of 2");
  }
  interned_ = std::make_unique<interned>(buckets);
}

auto intern_fixed_set::intern(std::string_view str) -> const std::string& {
  return interned_->intern(str);
}

auto intern_fixed_set::size() const -> unsigned int {
  return interned_->size();
}
