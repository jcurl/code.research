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
  ~statistics() = default;

  auto insert(value_type value) -> void;
  auto clear() -> void;

  [[nodiscard]] auto min() const noexcept -> value_type;
  [[nodiscard]] auto max() const noexcept -> value_type;
  [[nodiscard]] auto size() const noexcept -> unsigned long;
  [[nodiscard]] auto median() const noexcept -> value_type;
  [[nodiscard]] auto average() const noexcept -> value_type;

 private:
  std::multiset<value_type> set_{};
  unsigned long sum_{0};
};

#endif
