#include "dev-adder.h"

#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#include "osqnx/resmgr/device.h"
#include "stdext/expected.h"
#include "ubench/options.h"
#include "ubench/string.h"

static constexpr int rdev_major = 500;
const std::string basedev{"/dev/adder/"};

class driver {
 public:
  using attr_type = void;
  driver(unsigned int id) { init_devices(id); }

  operator bool() const noexcept { return true; }

  [[nodiscard]] auto get_config() const noexcept
      -> const os::qnx::resmgr::config& {
    return config_;
  }

  [[nodiscard]] auto get_devices() const noexcept
      -> const std::vector<os::qnx::resmgr::rsrc<attr_type>>& {
    return devices_;
  }

  auto devctl(resmgr_context_t* ctp, io_devctl_t* msg, RESMGR_OCB_T* ocb,
      [[maybe_unused]] attr_type* attr) noexcept -> int {
    if (ctp->rcvid == 0) return EINVAL;

    switch (msg->i.dcmd) {
      case DCMD_DRIVER_ADD: {
        if ((ocb->ioflag & (_IO_FLAG_RD | _IO_FLAG_WR)) !=
            (_IO_FLAG_RD | _IO_FLAG_WR))
          return EPERM;
        int lr = iofunc_devctl_verify_length<uint32_t>(ctp, msg);
        if (lr != EOK) return lr;

        auto value = devctl_data_cast<uint32_t*>(msg);
        (*value)++;

        return _RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + sizeof(uint32_t));
      }
      default:
        return _RESMGR_DEFAULT;
    }
  }

 private:
  os::qnx::resmgr::config config_{
      .self = false, .fixed_prio = false, .inherit_runmask = false};
  std::vector<os::qnx::resmgr::rsrc<attr_type>> devices_;

  auto init_devices(unsigned int core) noexcept -> void {
    std::string path = basedev + std::to_string(core);
    devices_.push_back(os::qnx::resmgr::rsrc<void>{//
        .path = std::string{path.c_str()},
        .order = os::qnx::resmgr::path_order::default_order,
        .mode = S_IFNAM | 0666,
        .uid = 0,
        .gid = 0,
        .rdev = makedev(0, rdev_major, core),
        .nbytes = 0,
        .attr = nullptr});
  }
};

class options {
 public:
  options(const options&) = delete;
  auto operator=(const options&) -> options& = delete;
  options(options&&) = default;
  auto operator=(options&&) -> options& = default;
  ~options() = default;

  [[nodiscard]] auto count() const noexcept -> unsigned int { return count_; }

  [[nodiscard]] auto start() const noexcept -> unsigned int { return start_; }

 private:
  options() = default;
  friend auto make_options(int argc, const char* const argv[]) noexcept
      -> stdext::expected<options, int>;

  unsigned int count_{std::thread::hardware_concurrency()};
  unsigned int start_{0};
};

[[nodiscard]] auto make_options(int argc, const char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  options o{};
  ubench::options opts{argc, argv, "i:n:?"};
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
        case '?': {
          std::cout << "dev-adder -n <threads> -i <start>" << std::endl;
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

auto driver_thread(unsigned int core) noexcept -> int {
  os::qnx::resmgr::device<driver> driver{core};

  auto result = driver.run();
  if (!result) {
    if (result.error() > 0) {
      std::cout << "Driver Exit Error: "
                << ubench::string::perror(result.error()) << std::endl;
    } else {
      std::cout << "Driver Exit Error: " << result.error() << std::endl;
    }
    return 255;
  } else {
    std::cout << "Exiting: " << *result << std::endl;
    return *result;
  }
}

auto main(int argc, char* argv[]) -> int {
  auto options = make_options(argc, argv);
  if (!options) return options.error();

  std::vector<std::thread> threads{};
  for (unsigned int i = 0; i < options->count(); i++) {
    auto core = options->start() + i;
    threads.emplace_back([core]() { driver_thread(core); });
  }

  for (auto& thread : threads) {
    thread.join();
  }
  return 0;
}
