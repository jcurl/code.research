#ifndef BENCHMARK_STRINTERN_H
#define BENCHMARK_STRINTERN_H

#include <forward_list>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>

/// @brief the base class for str_intern testing.
class str_intern {
 public:
  str_intern() = default;
  str_intern(const str_intern&) = delete;
  auto operator=(const str_intern&) -> str_intern& = delete;
  str_intern(str_intern&&) = default;
  auto operator=(str_intern&&) -> str_intern& = default;
  virtual ~str_intern() = default;

  /// @brief intern a given string and return the interned string.
  ///
  /// @param str the string to intern.
  ///
  /// @return the view of the interned string. It is one that either exists, or
  /// is copied and the copy is returned. This string is nul-terminated.
  virtual auto intern(std::string_view str) -> const std::string& = 0;

  /// @brief get the number of strings interned.
  ///
  /// @return the number of strings interned.
  [[nodiscard]] virtual auto size() const -> unsigned int = 0;
};

/// @brief a simple list of strings.
class intern_forward_list : public str_intern {
  /// @brief intern a given string and return the interned string.
  ///
  /// @param str the string to intern.
  ///
  /// @return the view of the interned string. It is one that either exists, or
  /// is copied and the copy is returned. This string is nul-terminated.
  auto intern(std::string_view str) -> const std::string& override;

  /// @brief get the number of strings interned.
  ///
  /// @return the number of strings interned.
  [[nodiscard]] auto size() const -> unsigned int override { return interned_; }

 private:
  unsigned int interned_{};
  std::forward_list<std::string> strings_{};
};

/// @brief a simple list of strings.
class intern_ordered_set : public str_intern {
  /// @brief intern a given string and return the interned string.
  ///
  /// @param str the string to intern.
  ///
  /// @return the view of the interned string. It is one that either exists, or
  /// is copied and the copy is returned. This string is nul-terminated.
  auto intern(std::string_view str) -> const std::string& override;

  /// @brief get the number of strings interned.
  ///
  /// @return the number of strings interned.
  [[nodiscard]] auto size() const -> unsigned int override {
    return set_.size();
  }

 private:
  std::set<std::string> set_{};
};

/// @brief a simple list of strings.
class intern_unordered_set : public str_intern {
  /// @brief intern a given string and return the interned string.
  ///
  /// @param str the string to intern.
  ///
  /// @return the view of the interned string. It is one that either exists, or
  /// is copied and the copy is returned. This string is nul-terminated.
  auto intern(std::string_view str) -> const std::string& override;

  /// @brief get the number of strings interned.
  ///
  /// @return the number of strings interned.
  [[nodiscard]] auto size() const -> unsigned int override {
    return set_.size();
  }

 private:
  std::unordered_set<std::string> set_{};
};

/// @brief do no interning.
class intern_none : public str_intern {
  /// @brief don't intern any string
  ///
  /// @param str the string to intern.
  ///
  /// @return an empty string.
  auto intern(std::string_view str) -> const std::string& override;

  /// @brief get the number of strings interned.
  ///
  /// @return the number of strings interned.
  [[nodiscard]] auto size() const -> unsigned int override { return 0; }
};

/// @brief A fixed set is a custom implementation implementing a set of a fixed
/// number of buckets.
class intern_fixed_set : public str_intern {
 public:
  intern_fixed_set() = delete;
  intern_fixed_set(const intern_fixed_set&) = delete;
  auto operator=(const intern_fixed_set&) -> intern_fixed_set& = delete;
  intern_fixed_set(intern_fixed_set&&) = default;
  auto operator=(intern_fixed_set&&) -> intern_fixed_set& = default;
  ~intern_fixed_set() override;

  /// @brief instantiate with a fixed number of buckets.
  ///
  /// @param buckets the number of buckets to use for the hash.
  ///
  /// @exception std::invalid_argument The buckets is not a power of 2.
  intern_fixed_set(std::size_t buckets);

  /// @brief intern a given string and return the interned string.
  ///
  /// @param str the string to intern.
  ///
  /// @return the view of the interned string. It is one that either exists, or
  /// is copied and the copy is returned. This string is nul-terminated.
  auto intern(std::string_view str) -> const std::string& override;

  /// @brief get the number of strings interned.
  ///
  /// @return the number of strings interned.
  [[nodiscard]] auto size() const -> unsigned int override;

 private:
  class interned;
  std::unique_ptr<interned> interned_;
};

#endif
