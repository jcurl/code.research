#ifndef BENCHMARK_STATISTICS_H
#define BENCHMARK_STATISTICS_H

#include <set>

class statistics {
 public:
  using value_type = unsigned int;

 public:
  explicit statistics() = default;
  statistics(const statistics& other) = delete;
  auto operator=(const statistics& other) -> statistics& = delete;
  statistics(statistics&& other) = delete;
  auto operator=(statistics&& other) -> statistics& = delete;

  auto insert(value_type value) -> void;
  auto clear() -> void;

  auto min() const noexcept -> value_type;
  auto max() const noexcept -> value_type;
  auto size() const noexcept -> unsigned long;
  auto median() const noexcept -> value_type;
  auto average() const noexcept -> value_type;

 private:
  std::multiset<value_type> set_{};
  unsigned long sum_{0};
};

#endif
