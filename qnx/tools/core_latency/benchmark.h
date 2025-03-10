#ifndef BENCHMARK_BASE_H
#define BENCHMARK_BASE_H

#include <cstdint>
#include <string>

class benchmark {
 public:
  benchmark() = default;
  benchmark(const benchmark& other) = default;
  benchmark(benchmark&& other) = default;
  virtual ~benchmark() = default;

  auto operator=(const benchmark& other) -> benchmark& = default;
  auto operator=(benchmark&& other) -> benchmark& = default;

  [[nodiscard]] virtual auto name() const -> std::string = 0;

  virtual auto init() -> bool { return true; };

  virtual auto run(std::uint32_t ping_core, std::uint32_t pong_core)
      -> std::uint32_t = 0;
};

#endif
