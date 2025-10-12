#include "osqnx/resmgr/device.h"

#include <sys/types.h>
#include <sys/iomsg.h>
#include <sys/resmgr.h>
#include <sys/stat.h>
#include <dirent.h>

#include <chrono>
#include <csignal>
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <thread>
#include <type_traits>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

auto strerror_ext(int errno_ext) -> std::string {
  std::stringstream str{};
  if (errno_ext < 0) {
    errno_ext = -errno_ext;

    const char* err_name;
    if (errno_ext < 400) {
      err_name = strerror(errno_ext);
    } else {
      switch (errno_ext) {
        case os::qnx::resmgr::ERUNNING:
          err_name = "Resource manager running";
          break;
        case os::qnx::resmgr::EDRVNOTREADY:
          err_name = "Resource manager driver not ready";
          break;
        case os::qnx::resmgr::EDRVNOPATH:
          err_name = "Resource manager driver registered no paths";
          break;
        default:
          err_name = "Unknown";
          break;
      }
    }

    str << "Error " << err_name << " (" << errno_ext << ")";
  } else {
    str << "Error " << errno_ext;
  }
  return str.str();
}

}  // namespace

class driver_null {
 public:
  using attr_type = void;

  driver_null(bool error_state = false) : error_state_{error_state} {
    devices_.push_back(os::qnx::resmgr::rsrc<attr_type>{//
        .path = std::string{"/dev/sample"},
        .order = os::qnx::resmgr::path_order::default_order,
        .mode = S_IFCHR | 0666,
        .uid = 0,
        .gid = 0,
        .rdev = 0,
        .nbytes = 0,
        .attr = nullptr});
  }

  auto uid() noexcept -> uid_t& { return devices_[0].uid; }

  auto gid() noexcept -> gid_t& { return devices_[0].gid; }

  auto mode() noexcept -> mode_t& { return devices_[0].mode; }

  auto rdev() noexcept -> dev_t& { return devices_[0].rdev; }

  auto nbytes() noexcept -> off_t& { return devices_[0].nbytes; }

  auto self() noexcept -> bool& { return config_.self; }

  driver_null(const driver_null&) = delete;
  auto operator=(const driver_null&) -> driver_null& = delete;
  driver_null(driver_null&&) = delete;
  auto operator=(driver_null&&) -> driver_null& = delete;
  ~driver_null() = default;

  operator bool() const noexcept { return !error_state_; }

  [[nodiscard]] auto get_config() const noexcept
      -> const os::qnx::resmgr::config& {
    return config_;
  }

  [[nodiscard]] auto get_devices() const noexcept
      -> const std::vector<os::qnx::resmgr::rsrc<attr_type>>& {
    return devices_;
  }

 private:
  bool error_state_{};
  os::qnx::resmgr::config config_{
      .self = true, .fixed_prio = false, .inherit_runmask = false};
  std::vector<os::qnx::resmgr::rsrc<attr_type>> devices_{};
};

TEST(resmgr_device, not_copyable) {
  ASSERT_FALSE(
      std::is_copy_constructible_v<os::qnx::resmgr::device<driver_null>>);
}

TEST(resmgr_device, not_copy_assignable) {
  ASSERT_FALSE(std::is_copy_assignable_v<os::qnx::resmgr::device<driver_null>>);
}

TEST(resmgr_device, not_movable) {
  ASSERT_FALSE(
      std::is_copy_constructible_v<os::qnx::resmgr::device<driver_null>>);
}

TEST(resmgr_device, not_move_assignable) {
  ASSERT_FALSE(std::is_move_assignable_v<os::qnx::resmgr::device<driver_null>>);
}

TEST(resmgr_device, is_device) {
  ASSERT_TRUE(os::qnx::resmgr::details::is_driver<driver_null>::value);
  ASSERT_TRUE(os::qnx::resmgr::details::is_driver_v<driver_null>);
}

TEST(resmgr_device, is_device_none) {
  class test {};

  class test2 {
    [[nodiscard]] auto get_config() const -> const os::qnx::resmgr::config&;
  };

  class test3 {
    [[nodiscard]] auto get_devices() const
        -> const std::vector<os::qnx::resmgr::rsrc<void>>&;
  };

  ASSERT_FALSE(os::qnx::resmgr::details::is_driver<test>::value);
  ASSERT_FALSE(os::qnx::resmgr::details::is_driver<test2>::value);
  ASSERT_FALSE(os::qnx::resmgr::details::is_driver<test3>::value);
}

// Checks the driver can be exited after 100ms, and returns the correct exit
// code without an error.
TEST(resmgr_device, start_exit_async) {
  os::qnx::resmgr::device<driver_null> driver{};
  ASSERT_FALSE(driver);

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);
  EXPECT_TRUE(std::filesystem::exists("/dev/sample"));

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(1);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 1);
}

TEST(resmgr_device, start_exit_before_run) {
  os::qnx::resmgr::device<driver_null> driver{};
  ASSERT_FALSE(driver);
  driver.request_exit(2);

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_FALSE(driver);
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  driver_thread.join();
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 2);
}

os::qnx::resmgr::device<driver_null> driver_signal1{};

void handle_signal(int signo) {
  if (signo == SIGUSR1) {
    driver_signal1.request_exit(3);
  }
}

TEST(resmgr_device, start_exit_in_signal) {
  ASSERT_FALSE(driver_signal1);

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver_signal1.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }

    // Will only get here when the driver has exited.
  });

  ASSERT_TRUE(driver_signal1.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver_signal1) << strerror_ext(error_code);
  EXPECT_TRUE(std::filesystem::exists("/dev/sample"));

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  auto old_signal = signal(SIGUSR1, handle_signal);
  raise(SIGUSR1);
  driver_thread.join();

  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 3);
  signal(SIGUSR1, old_signal);

  // Running again will not run the resource manager. It will just return the
  // last exit code.
  auto result2 = driver_signal1.run();
  EXPECT_TRUE(result2) << result2.error();
  if (result2) {
    EXPECT_EQ(*result2, 3);
  }
}

TEST(resmgr_device, start_driver_error_on_init) {
  os::qnx::resmgr::device<driver_null> driver{true};
  ASSERT_FALSE(driver);

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(1000)));
  ASSERT_FALSE(driver);
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  driver_thread.join();
  EXPECT_EQ(error_code, -os::qnx::resmgr::EDRVNOTREADY);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, config) {
  os::qnx::resmgr::device<driver_null> driver{};
  driver.driver().uid() = 1;
  driver.driver().gid() = 2;
  driver.driver().rdev() = makedev(0, 1, 1);
  driver.driver().nbytes() = 128;
  ASSERT_FALSE(driver);

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);
  EXPECT_TRUE(std::filesystem::exists("/dev/sample"));

  // Check the properties of the resource manager.
  struct stat statbuf {};
  int stat_result = stat("/dev/sample", &statbuf);
  ASSERT_EQ(stat_result, 0)
      << "Error: " << strerror(errno) << " (" << errno << ")";
  ASSERT_EQ(statbuf.st_uid, 1);
  ASSERT_EQ(statbuf.st_gid, 2);
  ASSERT_EQ(statbuf.st_mode, 0666 | S_IFCHR);
  ASSERT_EQ(statbuf.st_dev, makedev(0, 1, 1));
  ASSERT_EQ(statbuf.st_rdev, makedev(0, 1, 1));
  ASSERT_EQ(statbuf.st_size, 128);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, self) {
  os::qnx::resmgr::device<driver_null> driver{};

  // Can't connect within the same process context, so connections should fail.
  driver.driver().self() = false;

  ASSERT_FALSE(driver);

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  // The driver really does exist, we're just not allowed to connect to it from
  // the same process where it was instantiated.
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

struct driver_stats {
  int open;
  int close_dup;
  int close_ocb;
  int read;
  size_t readbytes;
  int pread;
  size_t preadoffset;
  int write;
  size_t writebytes;
  int pwrite;
  size_t pwriteoffset;
  int devctl;
  int lseek;
  uint32_t val;
  int devctl_status;

  auto reset() -> void { memset(this, 0, sizeof(struct driver_stats)); }
};

#define DCMD_DRIVER_ADD __DIOTF(_DCMD_MISC, 1, uint32_t)
#define DCMD_DRIVER_SET __DIOT(_DCMD_MISC, 2, uint32_t)
#define DCMD_DRIVER_GET __DIOF(_DCMD_MISC, 3, uint32_t)
#define DCMD_DRIVER_PING __DION(_DCMD_MISC, 4)
#define DCMD_DRIVER_PINGT __DIOT(_DCMD_MISC, 5, 0)
#define DCMD_DRIVER_PINGF __DIOF(_DCMD_MISC, 6, 0)
#define DCMD_DRIVER_UNIMPL __DIOTF(_DCMD_MISC, 15, 0)

class driver {
 public:
  using attr_type = driver_stats;

  driver() {
    devices_.push_back(os::qnx::resmgr::rsrc<attr_type>{//
        .path = std::string{"/dev/sample"},
        .order = os::qnx::resmgr::path_order::default_order,
        .mode = S_IFCHR | 0666,
        .uid = 1000,
        .gid = 1000,
        .rdev = 0,
        .nbytes = 0,
        .attr = &stats_});
  }
  driver(const driver&) = delete;
  auto operator=(const driver&) -> driver& = delete;
  driver(driver&&) = delete;
  auto operator=(driver&&) -> driver& = delete;
  ~driver() = default;

  operator bool() const noexcept { return true; }

  auto mode() noexcept -> mode_t& { return devices_[0].mode; }

  auto nbytes() noexcept -> off_t& { return devices_[0].nbytes; }

  auto stats() noexcept -> attr_type& { return stats_; }

  [[nodiscard]] auto get_config() const noexcept
      -> const os::qnx::resmgr::config& {
    return config_;
  }

  [[nodiscard]] auto get_devices() const noexcept
      -> const std::vector<os::qnx::resmgr::rsrc<attr_type>>& {
    return devices_;
  }

  auto open(resmgr_context_t* ctp, io_open_t* msg, RESMGR_HANDLE_T* handle,
      void* extra, attr_type* attr) noexcept -> int {
    attr->open++;
    return iofunc_open_default(ctp, msg, handle, extra);
  }

  auto close_ocb(resmgr_context_t* ctp, void* msg, RESMGR_OCB_T* ocb,
      attr_type* attr) noexcept -> int {
    attr->close_ocb++;
    return iofunc_close_ocb_default(ctp, msg, ocb);
  }

  auto close_dup(resmgr_context_t* ctp, io_close_t* msg, RESMGR_OCB_T* ocb,
      attr_type* attr) noexcept -> int {
    attr->close_dup++;
    return iofunc_close_dup_default(ctp, msg, ocb);
  }

  auto devctl(resmgr_context_t* ctp, io_devctl_t* msg, RESMGR_OCB_T* ocb,
      attr_type* attr) noexcept -> int {
    attr->devctl++;

    int status = iofunc_devctl_default(ctp, msg, ocb);
    if (status != _RESMGR_DEFAULT) return status;

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

        memset(&msg->o, 0, sizeof(msg->o));
        msg->o.ret_val = 63;
        return _RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + sizeof(uint32_t));
      }
      case DCMD_DRIVER_SET: {
        if ((ocb->ioflag & _IO_FLAG_WR) != _IO_FLAG_WR) return EPERM;
        int lr = iofunc_devctl_verify_length<uint32_t>(ctp, msg);
        if (lr != EOK) return lr;
        auto value = devctl_data_cast<uint32_t*>(msg);
        stats_.val = *value;
        msg->o.ret_val = 31;
        return EOK;
      }
      case DCMD_DRIVER_GET: {
        if ((ocb->ioflag & _IO_FLAG_RD) != _IO_FLAG_RD) return EPERM;
        int lr = iofunc_devctl_verify_length<uint32_t>(ctp, msg);
        if (lr != EOK) return lr;
        auto value = devctl_data_cast<uint32_t*>(msg);
        *value = stats_.val;

        memset(&msg->o, 0, sizeof(msg->o));
        msg->o.ret_val = 127;
        return _RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + sizeof(uint32_t));
      }
      case DCMD_DRIVER_PING: {
        msg->o.ret_val = 250;
        _RESMGR_STATUS(ctp, stats_.devctl_status);
        return _RESMGR_PTR(ctp, &msg->o, sizeof(msg->o));
      }
      case DCMD_DRIVER_PINGT: {
        msg->o.ret_val = 251;
        _RESMGR_STATUS(ctp, stats_.devctl_status);
        return _RESMGR_PTR(ctp, &msg->o, sizeof(msg->o));
      }
      case DCMD_DRIVER_PINGF: {
        msg->o.ret_val = 252;
        _RESMGR_STATUS(ctp, stats_.devctl_status);
        return _RESMGR_PTR(ctp, &msg->o, sizeof(msg->o));
      }
      default:
        return _RESMGR_DEFAULT;
    }
  }

  auto read(resmgr_context_t* ctp, io_read_t* msg, RESMGR_OCB_T* ocb,
      attr_type* attr) noexcept -> int {
    attr->read++;

    int status = iofunc_read_verify(ctp, msg, ocb, nullptr);
    if (status != EOK) return status;

    switch (msg->i.xtype & _IO_XTYPE_MASK) {
      case _IO_XTYPE_NONE:
        break;
      case _IO_XTYPE_OFFSET:
        attr->pread++;
        attr->preadoffset = as_pread_t(msg)->offset.offset;
        break;
      default:
        return _RESMGR_DEFAULT;
    }

    status = iofunc_read_verify_length(ctp, msg);
    if (status != EOK) return status;

    attr->readbytes = _IO_READ_GET_NBYTES(msg);

    // mark the access time as invalid (we just accessed it)
    ocb->attr->flags |= IOFUNC_ATTR_ATIME;

    // For an actual read, see
    // https://www.qnx.com/developers/docs/8.0/com.qnx.doc.neutrino.resmgr/topic/read_write_Sample_IO_READ.html

    _IO_SET_READ_NBYTES(ctp, 0);  // Always show we're at EOF.
    return _RESMGR_NPARTS(0);
  }

  auto write(resmgr_context_t* ctp, io_write_t* msg, RESMGR_OCB_T* ocb,
      attr_type* attr) noexcept -> int {
    attr->write++;

    int nonblock = 0;
    int status = iofunc_write_verify(ctp, msg, ocb, &nonblock);
    if (status != EOK) return status;

    switch (msg->i.xtype & _IO_XTYPE_MASK) {
      case _IO_XTYPE_NONE:
        break;
      case _IO_XTYPE_OFFSET:
        attr->pwrite++;
        attr->pwriteoffset = as_pwrite_t(msg)->offset.offset;
        break;
      default:
        return _RESMGR_DEFAULT;
    }

    status = iofunc_write_verify_length(ctp, msg);
    if (status != EOK) return status;

    attr->writebytes = _IO_WRITE_GET_NBYTES(msg);

    // Read data from the client, to show how it is done. We'll have a smaller
    // buffer just to show how to handle very large input messages.
    std::array<std::byte, 256> buffer{};
    std::size_t cursor = 0;
    while (true) {
      std::size_t remaining = attr->writebytes - cursor;
      std::size_t nbytes =
          remaining > buffer.size() ? buffer.size() : remaining;
      if (nbytes == 0) break;

      ssize_t r =
          resmgr_msgget(ctp, buffer.data(), nbytes, cursor + sizeof(msg->i));
      if (r == -1) return errno;
      cursor += r;
      if (static_cast<std::size_t>(r) < nbytes) break;
    }
    _IO_SET_WRITE_NBYTES(ctp, cursor);

    // mark the modify and create time as invalid (we just received a write it)
    ocb->attr->flags |= IOFUNC_ATTR_MTIME;

    return EOK;
  }

  auto lseek(resmgr_context_t* ctp, io_lseek_t* msg, RESMGR_OCB_T* ocb,
      attr_type* attr) -> int {
    attr->lseek++;
    return iofunc_lseek_default(ctp, msg, ocb);
  }

 private:
  driver_stats stats_{};
  os::qnx::resmgr::config config_{
      .self = true, .fixed_prio = false, .inherit_runmask = false};
  std::vector<os::qnx::resmgr::rsrc<attr_type>> devices_{};
};

TEST(resmgr_device, open_close) {
  os::qnx::resmgr::device<driver> driver{};

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, read) {
  os::qnx::resmgr::device<driver> driver{};

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  std::array<std::byte, 4096> buff{};
  int r = read(fd, buff.data(), buff.size());
  ASSERT_EQ(r, 0) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().read, 1);
  ASSERT_EQ(driver.driver().stats().pread, 0);
  ASSERT_EQ(driver.driver().stats().readbytes, buff.size());

  int pr = pread(fd, buff.data(), buff.size(), 10);
  ASSERT_EQ(pr, 0) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().read, 2);
  ASSERT_EQ(driver.driver().stats().pread, 1);
  ASSERT_EQ(driver.driver().stats().preadoffset, 10);

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  driver.driver().stats().reset();
  DIR* d = opendir("/dev/sample");
  ASSERT_EQ(d, nullptr);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, readblock) {
  os::qnx::resmgr::device<driver> driver{};

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  std::array<std::byte, 4096> buff{};
  int r = readblock(fd, 512, 0, 8, buff.data());
  ASSERT_EQ(r, 0) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().read, 1);
  ASSERT_EQ(driver.driver().stats().lseek, 1);
  ASSERT_EQ(driver.driver().stats().pread, 0);
  ASSERT_EQ(driver.driver().stats().readbytes, buff.size());

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  driver.driver().stats().reset();
  DIR* d = opendir("/dev/sample");
  ASSERT_EQ(d, nullptr);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, write_mod_readonly_perms) {
  os::qnx::resmgr::device<driver> driver{};
  driver.driver().mode() = 0444 | S_IFCHR;

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  // One could thinkg that because the permissions of the resource manager are
  // 0444, that a open("/dev/sample", O_RDWR), should fail.
  //
  // Because this test must run as "root" (to have the permissions to create the
  // resource manager), and as root, the QNX library call iofunc_check_access()
  // will not check the read/write permissions. And therefore it will succeed.
  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);

  // Further, as root, the write will also succeed by default.
  std::array<std::byte, 4096> buff{};
  int w = write(fd, buff.data(), buff.size());
  ASSERT_EQ(w, 4096) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().write, 1);
  ASSERT_EQ(driver.driver().stats().pwrite, 0);

  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, write_open_readonly) {
  os::qnx::resmgr::device<driver> driver{};

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDONLY);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);

  std::array<std::byte, 4096> buff{};
  int w = write(fd, buff.data(), buff.size());
  ASSERT_EQ(w, -1);
  ASSERT_EQ(errno, EBADF);  // Opened for readonly
  ASSERT_EQ(driver.driver().stats().write, 1);
  ASSERT_EQ(driver.driver().stats().pwrite, 0);

  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, write) {
  os::qnx::resmgr::device<driver> driver{};

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  std::array<std::byte, 4096> buff{};
  int w = write(fd, buff.data(), buff.size());
  ASSERT_EQ(w, 4096) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().write, 1);
  ASSERT_EQ(driver.driver().stats().pwrite, 0);
  ASSERT_EQ(driver.driver().stats().writebytes, buff.size());

  int pw = pwrite(fd, buff.data(), buff.size(), 20);
  ASSERT_EQ(pw, buff.size()) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().write, 2);
  ASSERT_EQ(driver.driver().stats().pwrite, 1);
  ASSERT_EQ(driver.driver().stats().pwriteoffset, 20);

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, writeblock) {
  os::qnx::resmgr::device<driver> driver{};

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  std::array<std::byte, 4096> buff{};
  int w = writeblock(fd, 512, 0, 8, buff.data());
  ASSERT_EQ(w, 8) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().write, 1);
  ASSERT_EQ(driver.driver().stats().lseek, 1);
  ASSERT_EQ(driver.driver().stats().pwrite, 0);
  ASSERT_EQ(driver.driver().stats().writebytes, buff.size());

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, lseek) {
  os::qnx::resmgr::device<driver> driver{};
  driver.driver().nbytes() = 100;

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  off_t l1 = lseek(fd, 10, SEEK_SET);
  ASSERT_EQ(l1, 10);
  ASSERT_EQ(driver.driver().stats().lseek, 1);

  off_t l2 = lseek(fd, 10, SEEK_CUR);
  ASSERT_EQ(l2, 20);
  ASSERT_EQ(driver.driver().stats().lseek, 2);

  off_t l3 = lseek(fd, -10, SEEK_END);
  ASSERT_EQ(l3, 90);  // File is 100 bytes.
  ASSERT_EQ(driver.driver().stats().lseek, 3);

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, devctl) {
  os::qnx::resmgr::device<driver> driver{};
  driver.driver().nbytes() = 100;

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  uint32_t ival = 42;
  int dstat = 255;
  int dres = devctl(fd, DCMD_DRIVER_ADD, &ival, sizeof(ival), &dstat);
  ASSERT_EQ(dres, EOK) << strerror_ext(-dres);
  ASSERT_EQ(driver.driver().stats().devctl, 1);
  ASSERT_EQ(ival, 43);
  ASSERT_EQ(dstat, 63);

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, devctl_perms) {
  os::qnx::resmgr::device<driver> driver{};
  driver.driver().nbytes() = 100;

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDONLY);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  uint32_t ival = 45;
  int dstat = 255;
  int dres = devctl(fd, DCMD_DRIVER_ADD, &ival, sizeof(ival), &dstat);
  ASSERT_EQ(dres, EPERM);
  ASSERT_EQ(driver.driver().stats().devctl, 1);
  ASSERT_EQ(ival, 45);
  ASSERT_EQ(dstat, 255);

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, devctl_bad_message) {
  os::qnx::resmgr::device<driver> driver{};
  driver.driver().nbytes() = 100;

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  uint32_t ival = 47;
  int dstat = 127;

  // We send a smaller packet than expected.
  int dres = devctl(fd, DCMD_DRIVER_ADD, &ival, 2, &dstat);
  ASSERT_EQ(dres, EBADMSG);
  ASSERT_EQ(driver.driver().stats().devctl, 1);
  ASSERT_EQ(ival, 47);
  ASSERT_EQ(dstat, 127);

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, devctl_not_impl) {
  os::qnx::resmgr::device<driver> driver{};
  driver.driver().nbytes() = 100;

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  int dstat = 127;

  // We send a smaller packet than expected.
  int dres = devctl(fd, DCMD_DRIVER_UNIMPL, nullptr, 0, &dstat);
  ASSERT_EQ(dres, ENOTTY) << strerror_ext(-dres);
  ASSERT_EQ(driver.driver().stats().devctl, 1);
  ASSERT_EQ(dstat, 127);

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, devctl_read) {
  os::qnx::resmgr::device<driver> driver{};
  driver.driver().nbytes() = 100;

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  driver.driver().stats().val = 1337;
  uint32_t ival;
  int dstat = 255;
  int dres = devctl(fd, DCMD_DRIVER_GET, &ival, sizeof(ival), &dstat);
  ASSERT_EQ(dres, EOK) << strerror_ext(-dres);
  ASSERT_EQ(driver.driver().stats().devctl, 1);
  ASSERT_EQ(ival, 1337);
  ASSERT_EQ(dstat, 127);

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, devctl_write) {
  os::qnx::resmgr::device<driver> driver{};
  driver.driver().nbytes() = 100;

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  uint32_t ival = 6502;
  int dres = devctl(fd, DCMD_DRIVER_SET, &ival, sizeof(ival), nullptr);
  ASSERT_EQ(dres, EOK) << strerror_ext(-dres);
  ASSERT_EQ(driver.driver().stats().devctl, 1);
  ASSERT_EQ(ival, 6502);
  ASSERT_EQ(driver.driver().stats().val, 6502);

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, devctl_ping) {
  os::qnx::resmgr::device<driver> driver{};
  driver.driver().nbytes() = 100;

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  int dstat = 255;
  int dres = devctl(fd, DCMD_DRIVER_PING, nullptr, 0, &dstat);
  ASSERT_EQ(dres, EOK) << strerror_ext(-dres);
  ASSERT_EQ(driver.driver().stats().devctl, 1);
  ASSERT_EQ(dstat, 250);

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, devctl_ping_f) {
  os::qnx::resmgr::device<driver> driver{};
  driver.driver().nbytes() = 100;

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  int dstat = 255;
  int dres = devctl(fd, DCMD_DRIVER_PINGF, nullptr, 0, &dstat);
  ASSERT_EQ(dres, EOK) << strerror_ext(-dres);
  ASSERT_EQ(driver.driver().stats().devctl, 1);
  ASSERT_EQ(dstat, 252);

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, devctl_ping_t) {
  os::qnx::resmgr::device<driver> driver{};
  driver.driver().nbytes() = 100;

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  int dstat = 255;
  int dres = devctl(fd, DCMD_DRIVER_PINGT, nullptr, 0, &dstat);
  ASSERT_EQ(dres, EOK) << strerror_ext(-dres);
  ASSERT_EQ(driver.driver().stats().devctl, 1);
  ASSERT_EQ(dstat, 251);

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}

TEST(resmgr_device, devctl_ping_status) {
  os::qnx::resmgr::device<driver> driver{};
  driver.driver().nbytes() = 100;

  int error_code = 0;
  int result_code = 0;
  std::thread driver_thread([&] {
    auto result = driver.run();
    if (!result) {
      error_code = result.error();
    } else {
      result_code = *result;
    }
  });

  ASSERT_TRUE(driver.wait_for(std::chrono::milliseconds(100)));
  ASSERT_TRUE(driver) << strerror_ext(error_code);

  int fd = open("/dev/sample", O_RDWR);
  ASSERT_NE(fd, -1) << strerror_ext(-errno);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 0);
  ASSERT_EQ(driver.driver().stats().close_ocb, 0);

  int dres;
  driver.driver().stats().devctl_status = 10;
  dres = devctl(fd, DCMD_DRIVER_PING, nullptr, 0, nullptr);
  ASSERT_EQ(dres, -10) << strerror_ext(-dres);
  ASSERT_EQ(driver.driver().stats().devctl, 1);

  driver.driver().stats().devctl_status = -EBADF;
  dres = devctl(fd, DCMD_DRIVER_PING, nullptr, 0, nullptr);
  ASSERT_EQ(dres, EBADF) << strerror_ext(-dres);
  ASSERT_EQ(driver.driver().stats().devctl, 2);

  close(fd);
  ASSERT_EQ(driver.driver().stats().open, 1);
  ASSERT_EQ(driver.driver().stats().close_dup, 1);
  ASSERT_EQ(driver.driver().stats().close_ocb, 1);

  // This is an asynchronous call. After the call, it may still be running, or
  // it may have actually exited. We don't know. We can only wait for the driver
  // thread to finish.
  driver.request_exit(0);
  driver_thread.join();
  EXPECT_FALSE(std::filesystem::exists("/dev/sample"));
  EXPECT_EQ(error_code, 0);
  EXPECT_EQ(result_code, 0);
}
