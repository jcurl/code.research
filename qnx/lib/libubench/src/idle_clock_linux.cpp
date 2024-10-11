#include <unistd.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <locale>
#include <memory>
#include <sstream>
#include <string>

#include "ubench/clock.h"

namespace ubench::chrono {

namespace {

class base_clock {
 public:
  base_clock() noexcept = default;
  base_clock(const base_clock& other) = delete;
  auto operator=(const base_clock& other) -> base_clock& = delete;
  base_clock(base_clock&& other) = delete;
  auto operator=(base_clock&& other) -> base_clock& = delete;
  virtual ~base_clock() = default;

  [[nodiscard]] virtual auto is_enabled() const noexcept -> bool {
    return false;
  }
  [[nodiscard]] virtual auto get_idle_clock() noexcept -> std::uint64_t {
    return 0;
  }
  [[nodiscard]] virtual auto type() const noexcept -> idle_clock_type {
    return idle_clock_type::null;
  }
};

class proc_idle_clock : public base_clock {
 public:
  proc_idle_clock() noexcept { init_proc_clock(); }
  proc_idle_clock(const proc_idle_clock& other) = delete;
  auto operator=(const proc_idle_clock& other) -> proc_idle_clock& = delete;
  proc_idle_clock(proc_idle_clock&& other) = delete;
  auto operator=(proc_idle_clock&& other) -> proc_idle_clock& = delete;
  ~proc_idle_clock() override = default;

  [[nodiscard]] auto is_enabled() const noexcept -> bool override {
    return cpu_stats_.is_open();
  }

  [[nodiscard]] auto get_idle_clock() noexcept -> std::uint64_t override {
    return get_idle_time();
  }

  [[nodiscard]] auto type() const noexcept -> idle_clock_type override {
    if (!cpu_stats_.is_open()) return idle_clock_type::null;
    return idle_clock_type::proc_stat;
  }

 private:
  long clock_ticks_per_sec_{0};
  std::ifstream cpu_stats_{};

  auto init_proc_clock() noexcept -> void {
    clock_ticks_per_sec_ = sysconf(_SC_CLK_TCK);
    if (clock_ticks_per_sec_ <= 0) {
      clock_ticks_per_sec_ = 0;
      return;
    }

    cpu_stats_.open("/proc/stat");
    if (!cpu_stats_) {
      cpu_stats_.close();
      clock_ticks_per_sec_ = 0;
      return;
    }

    // Ensure that we always interpret the /proc/stats correctly irrespective of
    // whatever locale the user has configured.
    cpu_stats_.imbue(std::locale::classic());
  }

  auto get_idle_time() noexcept -> std::uint64_t {
    if (!cpu_stats_.is_open()) return 0;

    std::string line;
    if (!std::getline(cpu_stats_, line)) return 0;

    std::istringstream iss(line);
    std::string token;
    int i = 0;
    while (iss >> token) {
      if (i == 4) {
        std::uint64_t idle_time = std::stoull(token);
        cpu_stats_.clear();
        cpu_stats_.seekg(0);

        // The idle_time is in units of jiffies. Need to convert this to
        // nanoseconds.
        return idle_time * 1000000000 / clock_ticks_per_sec_;
      }
      i++;
    }

    // Error, didn't find the contents. So close.
    cpu_stats_.close();
    return 0;
  }
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::unique_ptr<base_clock> linux_idle_clock{};

auto init_idle_clocks() -> bool {
  // This function should only be called by init_static(), so it is only
  // called once.
  std::unique_ptr<base_clock> proc_clock = std::make_unique<proc_idle_clock>();
  if (proc_clock->is_enabled()) {
    linux_idle_clock = std::move(proc_clock);
    return true;
  }

  linux_idle_clock = std::make_unique<base_clock>();
  return true;
}

auto init_static() -> bool {
  static const auto result = init_idle_clocks();
  return result;
}

}  // namespace

auto idle_clock::now() noexcept -> time_point {
  if (!init_static()) return time_point{duration(0)};

  std::uint64_t total_idle_time = linux_idle_clock->get_idle_clock();
  return time_point{std::chrono::duration_cast<duration>(
      std::chrono::nanoseconds(total_idle_time))};
}

auto idle_clock::is_available() noexcept -> bool { return init_static(); }

auto idle_clock::type() noexcept -> idle_clock_type {
  if (!init_static()) return idle_clock_type::null;
  return linux_idle_clock->type();
}

}  // namespace ubench::chrono
