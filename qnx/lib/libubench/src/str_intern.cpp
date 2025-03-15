#include "config.h"

#include "ubench/str_intern.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <forward_list>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>

#if HAVE_CXX_EXPERIMENTAL_MEMORY_RESOURCE
// Under QNX 7.1 (GCC 8.3.0), the functionality is only experimental. We map the
// experimental namespace to the std namespace to be compatible with later
// compiler collections.
#include <experimental/forward_list>
#include <experimental/memory_resource>
#include <experimental/vector>

namespace std::pmr {
using memory_resource = std::experimental::pmr::memory_resource;
template <class T>
using forward_list = std::experimental::pmr::forward_list<T>;
template <class T>
using vector = std::experimental::pmr::vector<T>;
}  // namespace std::pmr
#else
#include <memory_resource>
#include <vector>
#endif

namespace ubench::string {

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
  v++;
  return v;
}

/// @brief An extending monotonic memory resource.
template <std::size_t N>
class ext_monotonic_resource : public std::pmr::memory_resource {
 public:
  explicit ext_monotonic_resource() {
    mem_.emplace_front();
    mem_ptr_ = mem_.front().data();
  }
  ext_monotonic_resource(const ext_monotonic_resource&) = delete;
  auto operator=(const ext_monotonic_resource&)
      -> ext_monotonic_resource& = delete;
  ext_monotonic_resource(ext_monotonic_resource&&) = delete;
  auto operator=(ext_monotonic_resource&&) -> ext_monotonic_resource& = delete;
  ~ext_monotonic_resource() override {
    for (const auto p : mem_large_) {
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      delete[] static_cast<std::byte*>(p);
    }
  }

 private:
  static constexpr std::size_t BLOCK_SIZE = N;

  auto get_pointer(std::size_t bytes, std::size_t alignment) -> void* {
    void* p = std::align(alignment, bytes, mem_ptr_, avail_);
    if (p) {
      avail_ -= bytes;
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      mem_ptr_ = static_cast<std::byte*>(p) + bytes;
    }
    return p;
  }

  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  auto do_allocate(std::size_t bytes, std::size_t alignment) -> void* override {
    if (bytes > BLOCK_SIZE >> 4) {
      // Large chunks of memory will be allocated separately.
      void* l = (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
                    ? new (std::align_val_t(alignment)) std::byte[bytes]
                    : new std::byte[bytes];
      mem_large_.emplace_front(l);
      return l;
    }

    // Ensures we don't return the same pointer twice.
    if (bytes == 0) bytes = 1;

    void* p = get_pointer(bytes, alignment);
    if (p) return p;

    avail_ = BLOCK_SIZE;
    mem_.emplace_front();
    mem_ptr_ = mem_.front().data();
    p = get_pointer(bytes, alignment);
    if (p) return p;

    throw std::bad_alloc();
  }

  auto do_deallocate(void*, std::size_t, std::size_t) -> void override {
    return;
  }

  [[nodiscard]] auto do_is_equal(
      const std::pmr::memory_resource& other) const noexcept -> bool override {
    return this == &other;
  }

  std::size_t avail_{BLOCK_SIZE};
  void* mem_ptr_{};
  std::forward_list<std::array<std::byte, BLOCK_SIZE>> mem_{};
  std::forward_list<void*> mem_large_{};
};

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
class str_intern::interned {
 private:
  using alloc_s_t = ext_monotonic_resource<131072>;
  using alloc_h_t = ext_monotonic_resource<131072>;
  using string_block_t = std::pmr::forward_list<std::string>;
  using set_t = std::pmr::vector<std::pmr::forward_list<string_hash_block>>;

 public:
  interned(const interned&) = delete;
  auto operator=(const interned&) -> interned& = delete;
  interned(interned&&) = default;
  auto operator=(interned&&) -> interned& = default;
  ~interned() = default;

  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  interned(std::size_t buckets, std::size_t max_buckets)
      : hash_mask_(buckets - 1),
        max_buckets_(max_buckets),
        rehash_count_{buckets}  // Assumes a max_load_factor = 1.0
  {
    alloc_h_ = std::make_unique<alloc_h_t>();
    set_ = std::make_unique<set_t>(alloc_h_.get());
    set_->resize(buckets);
    if (buckets == max_buckets) rehash_count_ = 0;
  };

  [[nodiscard]] auto size() const -> unsigned int { return interned_; }

  auto intern(std::string_view str) -> const std::string& {
    std::size_t h = svh_(str);
    std::size_t i = h & hash_mask_;
    for (const auto& block : (*set_)[i]) {
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
    auto& interned = strings_->emplace_front(str);
    (*set_)[i].emplace_front(h, &interned);
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

    if (max_buckets_ == set_->size()) {
      // No need to recalculate if we're already at the maximum size. The
      // max_load_factor_ is then effectively ignored.
      rehash_count_ = 0;
    } else {
      float f = static_cast<float>(set_->size()) * max_load_factor_;
      if (f >= static_cast<float>(std::numeric_limits<std::size_t>::max())) {
        rehash_count_ = 0;
      } else {
        rehash_count_ =
            std::max(static_cast<std::size_t>(std::ceil(f)), set_->size());
      }
    }
  }

  [[nodiscard]] auto bucket_count() const -> std::size_t {
    return set_->size();
  }

 private:
  std::size_t interned_{};

  std::unique_ptr<alloc_s_t> alloc_s_ = std::make_unique<alloc_s_t>();
  std::unique_ptr<string_block_t> strings_ =
      std::make_unique<string_block_t>(alloc_s_.get());
  std::unique_ptr<alloc_h_t> alloc_h_{};
  std::unique_ptr<set_t> set_{};

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

    float f = static_cast<float>(interned_ + 1) / max_load_factor_;
    auto new_buckets = static_cast<std::size_t>(std::ceil(f));
    if (new_buckets >= max_buckets_) {
      return max_buckets_;
    }
    return bit_ceil(new_buckets);
  }

  auto rehash(std::size_t new_buckets) -> void {
    auto new_alloc = std::make_unique<alloc_h_t>();
    auto new_set = std::make_unique<set_t>(new_alloc.get());

    std::size_t hash_mask = new_buckets - 1;

    new_set->resize(new_buckets);
    for (auto& set : *set_) {
      for (const auto& block : set) {
        std::size_t i = block.hash & hash_mask;
        (*new_set)[i].emplace_front(block);
      }
    }

    hash_mask_ = hash_mask;

    // Replace the set first so that the old allocator is no longer needed. Then
    // replace the allocator.
    //
    // In C++17, it is undefined behaviour to assign a PMR container with
    // another one if the memory allocators are different. Because of this, we
    // need to give the memory allocator to the container during construction
    // (not assignment) and swap using pointers.
    set_ = std::move(new_set);
    alloc_h_ = std::move(new_alloc);

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
str_intern::str_intern(str_intern&&) noexcept = default;
auto str_intern::operator=(str_intern&&) noexcept -> str_intern& = default;
str_intern::~str_intern() = default;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
str_intern::str_intern(std::size_t buckets, std::size_t max_buckets) {
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

auto str_intern::intern(std::string_view str) -> const std::string& {
  return interned_->intern(str);
}

auto str_intern::size() const -> unsigned int { return interned_->size(); }

[[nodiscard]] auto str_intern::max_load_factor() const -> float {
  return interned_->max_load_factor();
}

auto str_intern::max_load_factor(float ml) -> void {
  if (ml <= 0.01) {
    throw std::invalid_argument("max_load_factor is too small");
  }
  interned_->max_load_factor(ml);
}

[[nodiscard]] auto str_intern::bucket_count() const -> std::size_t {
  return interned_->bucket_count();
}

}  // namespace ubench::string
