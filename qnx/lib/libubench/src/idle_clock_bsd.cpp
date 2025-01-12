#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <unistd.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "ubench/clock.h"
#include "base_clock.h"

namespace ubench::chrono {

namespace {

class cp_time_idle_clock : public base_clock {
 public:
  cp_time_idle_clock() noexcept { init_proc_clock(); }
  cp_time_idle_clock(const cp_time_idle_clock& other) = delete;
  auto operator=(const cp_time_idle_clock& other)
      -> cp_time_idle_clock& = delete;
  cp_time_idle_clock(cp_time_idle_clock&& other) = delete;
  auto operator=(cp_time_idle_clock&& other) -> cp_time_idle_clock& = delete;
  ~cp_time_idle_clock() override = default;

  [[nodiscard]] auto is_enabled() const noexcept -> bool override {
    return (clock_ticks_per_sec_ != 0);
  }

  [[nodiscard]] auto get_idle_clock() noexcept -> std::uint64_t override {
    return get_idle_time();
  }

  [[nodiscard]] auto type() const noexcept -> idle_clock_type override {
    if (clock_ticks_per_sec_ == 0) return idle_clock_type::null;
    return idle_clock_type::cptime;
  }

 private:
  long clock_ticks_per_sec_{};
  unsigned int ncpu_{};
  std::vector<std::uint64_t> times_{};
  std::array<int, 2> mib_cp_times_{};

  auto init_proc_clock() noexcept -> void {
    // Ask the Operating System tne number of CPUs. Don't use the CPP standard,
    // as that may be technically different to the way the kernel handles its
    // details.
    int mib[2];
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    std::size_t size = sizeof(ncpu_);
    if (sysctl(mib, 2, &ncpu_, &size, NULL, 0) < 0) return;
    if (ncpu_ > 0xFFFF) return;

    // Determine frequency of clock ticks, the resolution of the idle clock.
    clock_ticks_per_sec_ = sysconf(_SC_CLK_TCK);
    if (clock_ticks_per_sec_ <= 0) {
      clock_ticks_per_sec_ = 0;
    }

    // Allocate dynamic memory only once for getting CPU time information.
    times_.resize(ncpu_ * CPUSTATES);

    // Get the MIB for the KERN_CP_TIMES, as there is no constant available.
#if __NetBSD__
    mib_cp_times_[0] = CTL_KERN;
    mib_cp_times_[1] = KERN_CP_TIME;
#elif __FreeBSD__
    size_t len = mib_cp_times_.size();
    sysctlnametomib("kern.cp_times", mib_cp_times_.data(), &len);
#else
    // We don't know how to get the times, so initialise to unknown.
    clock_ticks_per_sec_ = 0;
#endif
  }

  auto get_idle_time() noexcept -> std::uint64_t {
    if (clock_ticks_per_sec_ == 0) return 0;

    // Get the clock details CP_* defined in <sys/sched.h>.
    std::size_t size = sizeof(decltype(times_)::value_type) * CPUSTATES * ncpu_;
    if (sysctl(mib_cp_times_.data(), mib_cp_times_.size(), times_.data(), &size,
            NULL, 0) < 0)
      return 0;

    // Sum up the idle ticks per CPU. Note that the accuracy of this system call
    // is not very high (and appears lower than Linux) as it's likely based on
    // sampling than a total time of idle based on an accumulative counter.
    // Better would be to investigate other modules for idle time measurements
    // than this.
    std::uint64_t idle_time{};
    for (std::size_t cpu = 0; cpu < ncpu_; cpu++) {
      idle_time += times_[cpu * CPUSTATES + CP_IDLE];
    }
    return 1000000000 / clock_ticks_per_sec_ * idle_time;
  }
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::unique_ptr<base_clock> bsd_idle_clock{};

auto init_idle_clocks() -> bool {
  // This function should only be called by init_static(), so it is only called
  // once. This implementation uses BSD, but it is designed that other
  // mechanisms can be used to get the idle clock, determined at run time on the
  // first invocation (e.g. via a kernel module, or linux emulation, etc.).
  std::unique_ptr<base_clock> proc_clock =
      std::make_unique<cp_time_idle_clock>();
  if (proc_clock->is_enabled()) {
    bsd_idle_clock = std::move(proc_clock);
    return true;
  }

  bsd_idle_clock = std::make_unique<base_clock>();
  return true;
}

auto init_static() -> bool {
  static const auto result = init_idle_clocks();
  return result;
}

}  // namespace

auto idle_clock::now() noexcept -> time_point {
  if (!init_static()) return time_point{duration(0)};

  std::uint64_t total_idle_time = bsd_idle_clock->get_idle_clock();
  return time_point{std::chrono::duration_cast<duration>(
      std::chrono::nanoseconds(total_idle_time))};
}

auto idle_clock::is_available() noexcept -> bool { return init_static(); }

auto idle_clock::type() noexcept -> idle_clock_type {
  if (!init_static()) return idle_clock_type::null;
  return bsd_idle_clock->type();
}

}  // namespace ubench::chrono
