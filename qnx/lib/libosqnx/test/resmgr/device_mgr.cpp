#include <sys/types.h>

#include <cerrno>
#include <iostream>
#include <string>
#include <vector>

#include "osqnx/resmgr/device.h"
#include "ubench/string.h"

class driver {
 private:
  const std::string s1{"s1"};
  const std::string s2{"s2"};

 public:
  using attr_type = const std::string;

  driver() = default;

  operator bool() const noexcept { return true; }

  [[nodiscard]] auto get_config() const noexcept
      -> const os::qnx::resmgr::config & {
    return config_;
  }

  [[nodiscard]] auto get_devices() const noexcept
      -> const std::vector<os::qnx::resmgr::rsrc<attr_type>> & {
    return devices_;
  }

  auto open([[maybe_unused]] resmgr_context_t *ctp,
      [[maybe_unused]] io_open_t *msg, [[maybe_unused]] RESMGR_HANDLE_T *handle,
      void *extra, [[maybe_unused]] attr_type *attr) noexcept -> int {
    std::cout << "this is my open: " << *attr << std::endl;
    return iofunc_open_default(ctp, msg, handle, extra);
  }

  auto devctl([[maybe_unused]] resmgr_context_t *ctp,
      [[maybe_unused]] io_devctl_t *msg, [[maybe_unused]] RESMGR_OCB_T *ocb,
      [[maybe_unused]] attr_type *attr) noexcept -> int {
    std::cout << "this is my devctl: " << *attr << std::endl;
    return ENOSYS;
  }

  auto read([[maybe_unused]] resmgr_context_t *ctp,
      [[maybe_unused]] io_read_t *msg, [[maybe_unused]] RESMGR_OCB_T *ocb,
      [[maybe_unused]] attr_type *attr) noexcept -> int {
    std::cout << "this is my read: " << *attr << std::endl;
    return ENOSYS;
  }

  auto write([[maybe_unused]] resmgr_context_t *ctp,
      [[maybe_unused]] io_write_t *msg, [[maybe_unused]] RESMGR_OCB_T *ocb,
      [[maybe_unused]] attr_type *attr) noexcept -> int {
    std::cout << "this is my write: " << *attr << std::endl;
    return ENOSYS;
  }

  auto lseek([[maybe_unused]] resmgr_context_t *ctp,
      [[maybe_unused]] io_lseek_t *msg, [[maybe_unused]] RESMGR_OCB_T *ocb,
      [[maybe_unused]] attr_type *attr) noexcept -> int {
    std::cout << "this is my lseek: " << *attr << std::endl;
    return iofunc_lseek_default(ctp, msg, ocb);
  }

  auto close_ocb([[maybe_unused]] resmgr_context_t *ctp,
      [[maybe_unused]] void *msg, [[maybe_unused]] RESMGR_OCB_T *ocb,
      [[maybe_unused]] attr_type *attr) noexcept -> int {
    std::cout << "this is my close_ocb: " << *attr << std::endl;
    return iofunc_close_ocb_default(ctp, msg, ocb);
  }

  auto close_dup([[maybe_unused]] resmgr_context_t *ctp,
      [[maybe_unused]] io_close_t *msg, [[maybe_unused]] RESMGR_OCB_T *ocb,
      [[maybe_unused]] attr_type *attr) noexcept -> int {
    std::cout << "this is my close_dup: " << *attr << std::endl;
    return iofunc_close_dup_default(ctp, msg, ocb);
  }

 private:
  os::qnx::resmgr::config config_{
      .self = false, .fixed_prio = false, .inherit_runmask = false};
  std::vector<os::qnx::resmgr::rsrc<attr_type>> devices_{
      os::qnx::resmgr::rsrc<attr_type>{//
          .path = std::string{"/dev/sample"},
          .order = os::qnx::resmgr::path_order::default_order,
          .mode = S_IFBLK | 0666,
          .uid = 0,
          .gid = 0,
          .rdev = makedev(0, 1, 0),
          .nbytes = 0,
          .attr = &s1}};
};

static os::qnx::resmgr::device<driver> driver{};

auto handle_signal(int signo) -> void { driver.request_exit(signo); }

auto main() -> int {
  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);

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
