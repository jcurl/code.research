#include <cstddef>
#include <forward_list>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "str_intern.h"

namespace {

struct string_hash_block {
  string_hash_block(std::size_t hash, std::string* interned)
      : hash{hash}, interned{interned} {}

  std::size_t hash;       //< The computed hash.
  std::string* interned;  //< Pointer to string that won't be freed.
};

/// @brief Calculate the next value of power of two.
///
/// Calculates the next value that is a power of 2. If the value is too large,
/// then zero is returned.
///
/// @tparam T the unsigned type to calculate the bit ceiling for.
///
/// @param v the value to calculate the bit ceiling for.
///
/// @return the next power of 2, or zero if none.
template <typename T,
    std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, bool> =
        true>
auto bit_ceil(T v) -> T {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  if (sizeof(T) > 1) {
    v |= v >> 8;
  }
  if (sizeof(T) > 2) {
    v |= v >> 16;
  }
  if (sizeof(T) > 4) {
    v |= v >> 32;
  }
  if (sizeof(T) > 8) {
    v |= v >> 64;
  }
  v++;
  return v;
}

}  // namespace

// Some details about the implementation.
//
// The rehash_count_ == buckets initially, because the max_load_factor_ is 1.0.
//
// When the rehash_count_ == 0, then no more checks should be made to increase
// the number of buckets. We've reached the maximum.
//
// load_factor = interned_ / buckets.
//
// max_load_factor = rehash_count_ / buckets.

/// @brief implementation of intern_var_set using the pimpl pattern.
class intern_var_set::interned {
 public:
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  interned(std::size_t buckets, std::size_t max_buckets)
      : hash_mask_(buckets - 1),
        max_buckets_(max_buckets),
        rehash_count_{buckets}  // Assumes a max_load_factor = 1.0
  {
    set_.resize(buckets);
    if (buckets == max_buckets) rehash_count_ = 0;
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
    if (rehash_count_ != 0 && interned_ >= rehash_count_) {
      rehash(next_size());

      // Because we've rehashed, the index to put this entry has also changed.
      i = h & hash_mask_;
    }

    interned_++;
    auto& interned = strings_.emplace_front(str);
    set_[i].emplace_front(h, &interned);
    return interned;
  }

  [[nodiscard]] auto max_load_factor() const -> float {
    return max_load_factor_;
  }

  /// @brief Update the max_load_factor.
  ///
  /// @param ml the new maximum load factor.
  auto max_load_factor(float ml) -> void {
    // As a reminder, the load_factor is based on the number of interned_ words.
    //
    //  load_factor = interned_ / buckets
    //
    // So that the max_load_factor_ can tell us how many interned_ words we need
    // before we need to rehash, such as:
    //
    //  max_load_factor_ = rehash_count_ / buckets
    //
    // so we want to recalculate what the new rehash_count_ should be.
    //
    //  rehash_count_ = max_load_factor_ * buckets
    //
    // If the rehash_count_ exceeds the maximum value, then we'll never rehash,
    // so we can set rehash_count_ to zero. If it is smaller than the current
    // value, we'll later have to update the size of the buckets the next time a
    // word is entered. We don't do that here.

    max_load_factor_ = ml;

    if (max_buckets_ == set_.size()) {
      // No need to recalculate if we're already at the maximum size. The
      // max_load_factor_ is then effectively ignored.
      rehash_count_ = 0;
    } else {
      float f = static_cast<float>(set_.size()) * max_load_factor_;
      if (f >= static_cast<float>(std::numeric_limits<std::size_t>::max())) {
        rehash_count_ = 0;
      } else {
        rehash_count_ = static_cast<std::size_t>(f);
      }
    }
  }

 private:
  std::size_t interned_{};
  std::vector<std::forward_list<string_hash_block>> set_{};
  std::forward_list<std::string> strings_{};
  std::size_t hash_mask_{};
  std::size_t max_buckets_{};
  float max_load_factor_{1.0};
  std::size_t rehash_count_{};
  std::hash<std::string_view> svh_{};

  [[nodiscard]] auto next_size() const -> std::size_t {
    // In calculating the number of buckets we want, we must fulfill the
    // following formula, so that we don't have to hash immediately again:
    //
    //  rehash_count_ > interned
    //
    // and
    //
    //  rehash_count_ = max_load_factor_ * buckets
    //
    // This results in
    //
    //  buckets > interned_ / max_load_factor_
    //
    // and then rounded up to the next power of 2.

    float f = static_cast<float>(interned_) / max_load_factor_;
    auto new_buckets = static_cast<std::size_t>(f);
    if (new_buckets >= max_buckets_) {
      return max_buckets_;
    }

    // Round the value up to the next power of 2. Choose a portable solution
    // instead of an optimised one (in C++20 we can use std::bit_ceil).
    if (new_buckets <= set_.size()) {
      // Due to rounding errors, we might not actually increase the size.
      return bit_ceil(new_buckets << 1);
    }
    return bit_ceil(new_buckets);
  }

  auto rehash(std::size_t new_buckets) -> void {
    std::vector<std::forward_list<string_hash_block>> new_set{};
    std::size_t hash_mask = new_buckets - 1;

    new_set.resize(new_buckets);
    for (auto& set : set_) {
      for (const auto& block : set) {
        std::size_t i = block.hash & hash_mask;
        new_set[i].emplace_front(block);
      }

      // Reduce the maximum amount of memory by freeing early.
      set.clear();
    }

    hash_mask_ = hash_mask;
    set_ = std::move(new_set);

    if (new_buckets == max_buckets_) {
      rehash_count_ = 0;
    } else {
      rehash_count_ = static_cast<std::size_t>(
          static_cast<float>(new_buckets) * max_load_factor_);
    }
  }
};

// For the "pimpl" pattern (using a unique_ptr on a forward declarated class),
// need to ensure that there is no definition (inline) of the constructor in the
// header file, else a move of "udp_talker_bpf" will not compile.
intern_var_set::~intern_var_set() = default;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
intern_var_set::intern_var_set(std::size_t buckets, std::size_t max_buckets) {
  if (buckets == 0 || buckets & (buckets - 1)) {
    throw std::invalid_argument("buckets is not a power of 2");
  }
  if (max_buckets < buckets) {
    throw std::invalid_argument("max_buckets must be greater/equal to buckets");
  }
  if (max_buckets & (buckets - 1)) {
    throw std::invalid_argument("max_buckets is not a power of 2");
  }
  interned_ = std::make_unique<interned>(buckets, max_buckets);
}

auto intern_var_set::intern(std::string_view str) -> const std::string& {
  return interned_->intern(str);
}

auto intern_var_set::size() const -> unsigned int { return interned_->size(); }

[[nodiscard]] auto intern_var_set::max_load_factor() const -> float {
  return interned_->max_load_factor();
}

auto intern_var_set::max_load_factor(float ml) -> void {
  if (ml <= 0.01) {
    throw std::invalid_argument("max_load_factor is too small");
  }
  interned_->max_load_factor(ml);
}
