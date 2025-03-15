#ifndef UBENCH_STR_INTERN_H
#define UBENCH_STR_INTERN_H

#include <memory>
#include <string>
#include <string_view>

namespace ubench::string {

/// @brief A fixed set is a custom implementation implementing a set of a
/// dynamic number of buckets.
class str_intern {
 public:
  str_intern() : str_intern(4096) {}
  str_intern(const str_intern&) = delete;
  auto operator=(const str_intern&) -> str_intern& = delete;
  str_intern(str_intern&&) noexcept;
  auto operator=(str_intern&&) noexcept -> str_intern&;
  ~str_intern();

  /// @brief instantiate with a fixed number of buckets.
  ///
  /// @param buckets Number of buckets to use initially.
  ///
  /// @param max_buckets Maximum number of buckets to use.
  ///
  /// @exception std::invalid_argument The parameter buckets or max_buckets is
  /// not a power of 2. The parameter max_buckets is less than buckets.
  str_intern(std::size_t buckets, std::size_t max_buckets = 1 << 20);

  /// @brief intern a given string and return the interned string.
  ///
  /// @param str the string to intern.
  ///
  /// @return the view of the interned string. It is one that either exists, or
  /// is copied and the copy is returned. This string is nul-terminated.
  auto intern(std::string_view str) -> const std::string&;

  /// @brief get the number of strings interned.
  ///
  /// @return the number of strings interned.
  [[nodiscard]] auto size() const -> unsigned int;

  /// @brief get the maximum load factor when rehashing.
  ///
  /// @return Returns current maximum load factor.
  [[nodiscard]] auto max_load_factor() const -> float;

  /// @brief set the maximum load factor when rehashing.
  ///
  /// The max load factor sets the upper limit when the number of buckets must
  /// be increased. This is the average number of values per bucket in the hash
  /// set. If greater than 1.0, then there are more than one string on average
  /// per bucket. If less than 1.0, then there is on average one string over
  /// 1/ml buckets (e.g. a load factor of 0.2 would be on average one string for
  /// five buckets).
  ///
  /// When changing the load factor so that more buckets are needed, the next
  /// interning operation will result in a rehash. When changing the load factor
  /// so that less buckets are needed (e.g. the value increases), then the
  /// number of buckets remains as it was.
  ///
  /// @param ml the new max load factor.
  auto max_load_factor(float ml) -> void;

  /// @brief Returns the number of buckets used for interning.
  ///
  /// @return the number of buckets allocated.
  [[nodiscard]] auto bucket_count() const -> std::size_t;

 private:
  class interned;
  std::unique_ptr<interned> interned_;
};

}  // namespace ubench::string

#endif
