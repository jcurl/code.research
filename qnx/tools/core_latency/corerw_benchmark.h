#ifndef BENCHMARK_CORE_RW_H
#define BENCHMARK_CORE_RW_H

#include <atomic>
#include <cstdint>
#include <string>

#include "benchmark.h"

class corerw_benchmark : public benchmark {
 public:
  corerw_benchmark() = default;
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  corerw_benchmark(std::uint32_t iterations, std::uint32_t samples)
      : iterations_{iterations}, samples_{samples} {}

  auto name() const -> std::string override;

  auto init() -> void override;

  auto run(std::uint32_t ping_core, std::uint32_t pong_core)
      -> std::uint32_t override;

 private:
  static constexpr std::uint32_t PING = 0;
  static constexpr std::uint32_t PONG = !PING;
  std::uint32_t iterations_{4000};
  std::uint32_t samples_{500};

  // Starting from Intel's Sandy Bridge, spatial prefetcher is now pulling pairs
  // of 64-byte cache lines at a time, so we have to align to 128 bytes rather
  // than 64.
  alignas(128) std::atomic<std::uint32_t> ping_;
  alignas(128) std::atomic<std::uint32_t> pong_;
};

#endif
