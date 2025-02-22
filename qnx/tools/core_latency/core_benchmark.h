#ifndef BENCHMARK_CORE_H
#define BENCHMARK_CORE_H

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

#include "benchmark.h"

enum class cas_type { cpp, x86, x86_64, arm64, arm64_lse };

class core_benchmark : public benchmark {
 public:
  core_benchmark() = default;

  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  core_benchmark(std::uint32_t iterations, std::uint32_t samples,
      cas_type ctype = cas_type::cpp)
      : iterations_{iterations}, samples_{samples}, ctype_{ctype} {}

  [[nodiscard]] auto name() const -> std::string override;

  [[nodiscard]] auto init() -> bool override;

  auto run(std::uint32_t ping_core, std::uint32_t pong_core)
      -> std::uint32_t override;

  /// @brief Get a list of supported benchmarks at compile time.
  ///
  /// @return a map to get the list of supported tests found at compile time.
  [[nodiscard]] static auto supported() noexcept
      -> const std::unordered_map<std::string_view, cas_type>&;

  /// @brief Get the CAS type for instantiating this class from a string.
  ///
  /// @return the enumeration value to instantiate with, given a string.
  [[nodiscard]] static auto mode(std::string_view benchmark) noexcept
      -> std::optional<cas_type>;

 private:
  static constexpr std::uint32_t PING = 0;
  static constexpr std::uint32_t PONG = 1;
  std::atomic<std::uint32_t> flag_{PING};
  std::uint32_t iterations_{4000};
  std::uint32_t samples_{500};
  cas_type ctype_{cas_type::cpp};
};

#endif
