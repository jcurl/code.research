#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

#include "stdext/expected.h"
#include "ubench/clock.h"
#include "ubench/measure/busy_measurement.h"
#include "ubench/options.h"
#include "ubench/string.h"
#include "ubench/thread.h"
#include "dev-adder.h"

using namespace std::chrono_literals;

const std::string basedev{"/dev/adder/"};

class options {
 public:
  options(const options&) = delete;
  auto operator=(const options&) -> options& = delete;
  options(options&&) = default;
  auto operator=(options&&) -> options& = default;
  ~options() = default;

  [[nodiscard]] auto count() const noexcept -> unsigned int { return count_; }

  [[nodiscard]] auto start() const noexcept -> unsigned int { return start_; }

  [[nodiscard]] auto iterations() const noexcept -> unsigned int {
    return iterations_;
  }

  [[nodiscard]] auto idle() const noexcept -> bool { return idle_; }

 private:
  options() = default;
  friend auto make_options(int argc, const char* const argv[]) noexcept
      -> stdext::expected<options, int>;

  unsigned int count_{std::thread::hardware_concurrency()};
  unsigned int start_{0};
  unsigned int iterations_{2000000};
  bool idle_{false};
};

[[nodiscard]] auto make_options(int argc, const char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  options o{};
  ubench::options opts{argc, argv, "i:n:IS:?"};
  for (const auto& opt : opts) {
    if (opt) {
      switch (opt->get_option()) {
        case 'n': {
          auto n = ubench::string::parse_int<unsigned int>(*opt->argument());
          if (!n || *n < 1 || *n > 128) {
            std::cerr << "Error: Invalid value for number of devices - "
                      << *opt->argument() << std::endl;
            return stdext::unexpected{1};
          }
          o.count_ = *n;
          break;
        }
        case 'i': {
          auto i = ubench::string::parse_int<unsigned int>(*opt->argument());
          if (!i) {
            std::cerr << "Error: Invalid value for start device - "
                      << *opt->argument() << std::endl;
            return stdext::unexpected{1};
          }
          o.start_ = *i;
          break;
        }
        case 'S': {
          auto s = ubench::string::parse_int<unsigned int>(*opt->argument());
          if (!s || s < 100) {
            std::cerr << "Error: Invalid value samples - " << *opt->argument()
                      << std::endl;
            return stdext::unexpected{1};
          }
          o.iterations_ = *s;
          break;
        }
        case 'I': {
          o.idle_ = true;
          break;
        }
        case '?': {
          std::cout << "adder -n <threads> -i <start> -I -S <samples>"
                    << std::endl;
          return stdext::unexpected{0};
        }
        default:
          ubench::options::print_error(opt->get_option());
          return stdext::unexpected{1};
      }
    }
  }
  if (o.start_ > std::numeric_limits<unsigned int>::max() - o.count_) {
    std::cerr << "Error: Overflow of 32-bit with -n and -i" << std::endl;
    return stdext::unexpected{1};
  }
  return o;
}

class measurement {
 public:
  measurement(const options& opt)
      : start_{opt.start()},
        iterations_{opt.iterations()},
        count_{opt.count()} {
    measurements_.resize(opt.count());
  }

  auto ready() -> void {
    unsigned int next;
    unsigned int expected;
    do {
      expected = ready_.load();
      next = expected + 1;
    } while (!std::atomic_compare_exchange_strong(&ready_, &expected, next));
    if (next == count_) {
      ready_event_.set();
    }
  }

  auto waitfor_ready() -> void { ready_event_.wait(); }

  auto run() -> void { run_event_.set(); }

  auto waitfor_run() -> void { run_event_.wait(); }

  [[nodiscard]] auto start() const noexcept -> unsigned int { return start_; }

  [[nodiscard]] auto iterations() const noexcept -> unsigned int {
    return iterations_;
  }

  [[nodiscard]] auto count() const noexcept -> unsigned int { return count_; }

  auto set_time(unsigned int core, std::chrono::milliseconds d) noexcept {
    measurements_[core] = d;
  }

  [[nodiscard]] auto times() const noexcept
      -> const std::vector<std::chrono::milliseconds>& {
    return measurements_;
  }

 private:
  unsigned int start_;
  unsigned int iterations_;
  unsigned int count_;
  std::atomic<unsigned int> ready_{0};
  ubench::thread::sync_event ready_event_{};
  ubench::thread::sync_event run_event_{};
  std::vector<std::chrono::milliseconds> measurements_{};
};

auto idle_test(std::chrono::milliseconds duration) noexcept
    -> std::optional<ubench::measure::busy_measurement> {
  if (!ubench::chrono::idle_clock::is_available()) return {};
  std::this_thread::sleep_for(500ms);
  {
    ubench::measure::busy_stop_watch measure{};
    std::this_thread::sleep_for(duration);

    auto idle_measurement = measure.measure();
    return idle_measurement;
  }
}

auto driver_thread(measurement& m, unsigned int core) -> int {
  std::string path = basedev + std::to_string(m.start() + core);
  int fd = open(path.c_str(), O_RDWR);
  if (fd == -1) {
    m.ready();
    ubench::string::perror(path);
    return 1;
  }

  std::uint32_t value = 0;
  m.ready();
  m.waitfor_run();
  auto start_time = std::chrono::high_resolution_clock::now();

  // Run over all iterations
  while (value < m.iterations()) {
    int r = devctl(fd, DCMD_DRIVER_ADD, &value, sizeof(value), nullptr);
    if (r != EOK) {
      errno = r;
      ubench::string::perror(path);
      close(fd);
      return 2;
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto time_span = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);
  m.set_time(core, time_span);
  std::cout << "Thread " << core << " done..." << std::endl;
  close(fd);
  return 0;
}

auto main(int argc, char* argv[]) -> int {
  auto options = make_options(argc, argv);
  if (!options) return options.error();

  std::optional<ubench::measure::busy_measurement> idle_measurement{};
  if (options->idle()) {
    std::cout << "Performing IDLE test for 5s... " << std::flush;
    idle_measurement = idle_test(5000ms);
    if (idle_measurement) {
      std::cout << "done." << std::endl;
    } else {
      std::cout << "failed." << std::endl;
    }
  }

  auto m = measurement{*options};
  std::vector<std::thread> threads{};
  for (unsigned int i = 0; i < options->count(); i++) {
    auto core = options->start() + i;
    std::cout << "Thread " << core << " initialising "
              << ". Iterating for " << m.iterations() << "..." << std::endl;
    threads.emplace_back([&m, core]() { driver_thread(m, core); });
  }

  m.waitfor_ready();
  // All threads have now initialised and waiting for run.
  std::this_thread::sleep_for(100ms);

  ubench::measure::busy_stop_watch sw{};
  m.run();

  for (auto& thread : threads) {
    thread.join();
  }
  auto mr = sw.measure();

  std::cout << "Results:" << std::endl;
  std::cout << "--------" << std::endl;
  if (options->idle() && idle_measurement) {
    std::uint32_t baseline = idle_measurement->busy_time.count() * 10000 /
                             idle_measurement->run_time.count();
    std::cout << "Total CPU Busy (Idle Test): " << baseline / 100 << "."
              << std::setw(2) << std::setfill('0') << baseline % 100 << "%"
              << std::endl;
  }
  std::uint32_t busy = mr.busy_time.count() * 10000 / mr.run_time.count();
  std::cout << "Total CPU Busy: " << busy / 100 << "." << std::setw(2)
            << std::setfill('0') << busy % 100 << "%" << std::endl;
  for (auto& time : m.times()) {
    float rate = options->iterations() * 1000.0 / time.count();
    std::cout << "Thread duration: " << time.count() << "ms (" << rate
              << " msgs/sec)" << std::endl;
  }
  return 0;
}
