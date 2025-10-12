#ifndef LIB_OSQNX_DEVICE_RESOURCE_MANAGER_H
#define LIB_OSQNX_DEVICE_RESOURCE_MANAGER_H

// Must be included before dispatch.h and resmgr.h so that RESMGR_HANDLE_T and
// RESMGR_OCB_T are properly defined.
#undef RESMGR_HANDLE_T
#undef RESMGR_OCB_T
#include <sys/iofunc.h>

// Must come next, else RESMGR_HANDLE_T and RESMGR_OCT_T would be void *.
#include <sys/dispatch.h>
#include <sys/resmgr.h>
#include <devctl.h>

#include <atomic>
#include <cerrno>
#include <string>
#include <type_traits>
#include <vector>

#include "osqnx/qnxver.h"
#include "stdext/expected.h"
#include "ubench/thread.h"

namespace os::qnx::resmgr {

constexpr int ERUNNING = 400;      ///< The resource manager is already running.
constexpr int EDRVNOTREADY = 401;  ///< The driver returns an error.
constexpr int EDRVNOPATH = 402;  ///< No resource paths provided by the driver.

/// @brief Defines the order the path is called by the QNX path manager.
enum class path_order {
  default_order,  ///< The default position.
  before,         ///< Path is installed before all other paths.
  after           ///< Path is installed after all other paths.
};

/// @brief Defines an individual resource.
///
/// @tparam TAttr your attribute type that is given to the callbacks for
/// resource manager functions.
template <typename TAttr = void>
struct rsrc {
  std::string path;  ///< Path to the resource.
  path_order order;  ///< Order to mount the resource.
  mode_t mode;       ///< Initial access mode of the resource.
  uid_t uid;         ///< User ID for the owner.
  gid_t gid;         ///< Group ID for the group access.
  dev_t rdev;        ///< Device number or rdev number.
  off_t nbytes;      ///< Number of bytes.
  TAttr* attr;       ///< User provided handler function callback attributes.
};

/// @brief Configuration of the resource manager
struct config {
  bool self;             ///< If should be visible to self (e.g. unit testing).
  bool fixed_prio;       ///< Channel doesn't inherit priority of client.
  bool inherit_runmask;  ///< Thread inherits clients run mask.
};

namespace details {

template <typename TDriver, typename = std::void_t<>>
struct is_driver : std::false_type {};

template <typename TDriver>
struct is_driver<TDriver,
    std::void_t<decltype(std::declval<TDriver>().get_config()),
        decltype(std::declval<TDriver>().get_devices())>> : std::true_type {};

template <typename TDriver>
inline constexpr bool is_driver_v = is_driver<TDriver>::value;

/// @brief A generic QNX single threaded resource manager.
class device {
 private:
  static constexpr int DISPATCH_NO_EXIT = 0;
  static constexpr int DISPATCH_EXIT_REQUESTED = 1;

 public:
  device() = default;
  device(const device&) = delete;
  auto operator=(const device&) -> device& = delete;
  device(device&&) = delete;
  auto operator=(device&&) -> device& = delete;
  ~device() = default;

  /// @brief Start the dispatch loop.
  template <typename TDriver, typename TAttr>
  auto run(TDriver& driver, const config& cfg,
      const std::vector<rsrc<TAttr>>& paths,
      const resmgr_connect_funcs_t& connect,
      const resmgr_io_funcs_t& io) noexcept -> stdext::expected<int, int> {
    auto& t = reinterpret_cast<const std::vector<rsrc<void>>&>(paths);
    return run_internal(&driver, cfg, &t, &connect, &io);
  }

  /// @brief Set an exit code, exiting the dispatch loop from run().
  ///
  /// This method is signal safe.
  auto exit(int code) noexcept -> void;

  /// @brief Wait for the dispatch loop to start, or run() to exit.
  auto wait() -> void { running_event_.wait(); }

  /// @brief Wait for the dispatch loop to start, or run() to exit, with a
  /// timeout.
  template <class Rep, class Period>
  auto wait_for(const std::chrono::duration<Rep, Period>& rel_time) -> bool {
    return running_event_.wait_for(rel_time);
  }

  /// @brief Indicate that wait() and wait_for() should return.
  ///
  /// Useful in case of an error occurs before run() is called.
  auto wait_set() -> void { running_event_.set(); }

  /// @brief checks if exit() was called.
  ///
  /// Only set by the exit() method.
  auto is_exited() const noexcept -> bool {
    return exit_requested_.load() != DISPATCH_NO_EXIT;
  }

  /// @brief Get the exit code called to exit(), or 0 if never called.
  auto exit_code() const noexcept -> int { return exit_code_; }

  /// @brief Checks if the dispatch loop is running.
  auto is_running() const noexcept -> bool { return running_.load(); }

 private:
  // run() method.
  dispatch_t* dpp_{};
  dispatch_context_t* ctp_{};

  // exit() method.
  ubench::thread::sync_event running_event_{};
  std::atomic<bool> running_{false};
  std::atomic<int> exit_requested_{DISPATCH_NO_EXIT};
  int exit_code_{0};

  auto run_internal(void* driver, const config& cfg,
      const std::vector<rsrc<>>* paths, const resmgr_connect_funcs_t* connect,
      const resmgr_io_funcs_t* io) noexcept -> stdext::expected<int, int>;
  auto free(const std::vector<int> ids, std::size_t count) noexcept -> void;
};

struct iofunc_attr_ext_t {
  iofunc_attr_t attr;
  void* driver;
  void* user_attr;
};

template <typename TDriver, typename TMsg, typename TMsgExtra, typename TAttr,
    int (TDriver::*Func)(
        resmgr_context_t*, TMsg*, RESMGR_HANDLE_T*, TMsgExtra*, TAttr*)>
auto resmgr_connect_handler(resmgr_context_t* ctp, TMsg* msg,
    RESMGR_HANDLE_T* handle, TMsgExtra* extra) noexcept -> int {
  auto dattr = reinterpret_cast<iofunc_attr_ext_t*>(handle);
  auto driver = reinterpret_cast<TDriver*>(dattr->driver);
  auto attr = reinterpret_cast<TAttr*>(dattr->user_attr);
  return (driver->*Func)(ctp, msg, handle, extra, attr);
}

template <typename TDriver, typename TMsg, typename TAttr,
    int (TDriver::*Func)(resmgr_context_t*, TMsg*, RESMGR_OCB_T*, TAttr*)>
auto resmgr_iofunc_handler(
    resmgr_context_t* ctp, TMsg* msg, RESMGR_OCB_T* ocb) noexcept -> int {
  auto dattr = reinterpret_cast<iofunc_attr_ext_t*>(ocb->attr);
  auto driver = reinterpret_cast<TDriver*>(dattr->driver);
  auto attr = reinterpret_cast<TAttr*>(dattr->user_attr);
  return (driver->*Func)(ctp, msg, ocb, attr);
}

}  // namespace details

/// @brief QNX message type for pread extended type requests.
struct io_pread_t {
  struct _io_read read;
  struct _xtype_offset offset;
};

/// @brief Conveniently interpret msg as a io_pread_t to get the offset
/// information.
///
/// When NDEBUG is not defined, this method will abort if the message is not an
/// _IO_XTYPE_OFFSET message.
///
/// @param msg The io_read_t message given in the read callback.
auto inline as_pread_t(io_read_t* msg) noexcept -> io_pread_t* {
#ifndef NDEBUG
  if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_OFFSET) std::abort();
#endif
  return reinterpret_cast<io_pread_t*>(msg);
}

/// @brief QNX message type for pwrite extended type requests.
struct io_pwrite_t {
  struct _io_write write;
  struct _xtype_offset offset;
};

/// @brief Conveniently interpret msg as a io_pread_t to get the offset
/// information.
///
/// When NDEBUG is not defined, this method will abort if the message is not an
/// _IO_XTYPE_OFFSET message.
///
/// @param msg The io_write_t message given in the write callback.
auto inline as_pwrite_t(io_write_t* msg) noexcept -> io_pwrite_t* {
#ifndef NDEBUG
  if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_OFFSET) std::abort();
#endif
  return reinterpret_cast<io_pwrite_t*>(msg);
}

/// @brief Check the write message, that the data to write doesn't exceed the
/// message length.
///
/// This condition should never fail, unless the OS maps less data than what it
/// says is available. This could occur if an application constructs the message
/// without libc.
///
/// @param ctp The resource manager context.
///
/// @param msg The write message.
///
/// @returns the error code. If EOK, then checks passed, else return the error
/// code from the handler.
auto inline iofunc_write_verify_length(
    resmgr_context_t* ctp, io_write_t* msg) noexcept -> int {
  size_t len = _IO_WRITE_GET_NBYTES(msg);
  if (len > static_cast<std::size_t>(ctp->info.srcmsglen - ctp->offset) -
                sizeof(io_write_t))
    return EBADMSG;
  return EOK;
}

/// @brief Check the read message, that the data to read doesn't exceed the
/// message length.
///
/// This condition should never fail, unless the OS maps less data than what it
/// says is available. This could occur if an application constructs the message
/// without libc.
///
/// @param ctp The resource manager context.
///
/// @param msg The write message.
///
/// @returns the error code. If EOK, then checks passed, else return the error
/// code from the handler.
auto inline iofunc_read_verify_length(
    resmgr_context_t* ctp, io_read_t* msg) noexcept -> int {
  size_t len = _IO_READ_GET_NBYTES(msg);
  if (len > static_cast<std::size_t>(ctp->info.dstmsglen)) return EINVAL;
  return EOK;
}

/// @brief Check the devctl message input data (_DIOT).
///
/// Messages that receive data are checked that there is sufficient data in the
/// input buffer. If not, then EBADMSG is returned. Messages that reply with
/// data are checked that there is sufficient data in the output buffer. If not,
/// then EINVAL is returned.
///
/// @tparam T The type provided as the input (fixed size).
///
/// @param ctp The resource manager context.
///
/// @param msg The devctl message.
///
/// @returns the error code. If EOK, then checks passed, else return the error
/// code from the handler.
template <typename T>
auto inline iofunc_devctl_verify_length(
    resmgr_context_t* ctp, io_devctl_t* msg) noexcept -> int {
  auto dir = get_device_direction(msg->i.dcmd);

  if (dir == DEVDIR_TO || dir == DEVDIR_TOFROM) {
    if (static_cast<std::size_t>(ctp->info.srcmsglen) <
        sizeof(struct _io_devctl) + sizeof(T))
      return EBADMSG;
  }

  if (dir == DEVDIR_FROM || dir == DEVDIR_TOFROM) {
    if (static_cast<std::size_t>(ctp->info.dstmsglen) <
        sizeof(struct _io_devctl_reply) + sizeof(T))
      return EINVAL;
  }

  return EOK;
}

/// @brief Typecast the input message data.
///
/// This is dependent on the QNX version in use. For QNX7, the old mechanism
/// _DEVCTL_DATA is used, which is no longer recommended for QNX8 and later.
///
/// @tparam T The type to cast the input field to.
///
/// @param msg The message received from the devctl() handler callback.
///
/// @returns The user data field of the incoming message.
template <typename T>
auto inline devctl_data_cast(io_devctl_t* msg) noexcept -> T {
#if QNXVER >= 800
  return static_cast<T>(_IO_INPUT_PAYLOAD(msg));
#else
  return static_cast<T>(_DEVCTL_DATA(msg->i));
#endif
}

/// @brief A resource manager implementation for single path devices.
///
/// This class suitable for creating a resource manager that handles individual
/// paths. It is not suitable as a file system driver.
///
/// @tparam TDriver the driver implementation providing the resource manager
/// functions
///
/// @tparam TAttr the attribute type in the callback functions. Define in your
/// TDriver::attr_type the type for the callback function.
template <typename TDriver, typename TAttr = typename TDriver::attr_type,
    std::enable_if_t<details::is_driver_v<TDriver>, bool> = true>
class device {
 private:
  static constexpr int DISPATCH_NO_EXIT = 0;
  static constexpr int DISPATCH_EXIT_REQUESTED = 1;

  template <typename T, typename = std::void_t<>>
  struct has_open : std::false_type {};

  template <typename T>
  struct has_open<T,
      std::void_t<decltype(static_cast<int (T::*)(resmgr_context_t*, io_open_t*,
              RESMGR_HANDLE_T*, void*, TAttr*)>(&T::open))>> : std::true_type {
  };

  template <typename T, typename = std::void_t<>>
  struct has_devctl : std::false_type {};

  template <typename T>
  struct has_devctl<T,
      std::void_t<decltype(static_cast<int (T::*)(resmgr_context_t*,
              io_devctl_t*, RESMGR_OCB_T*, TAttr*)>(&T::devctl))>>
      : std::true_type {};

  template <typename T, typename = std::void_t<>>
  struct has_read : std::false_type {};

  template <typename T>
  struct has_read<T,
      std::void_t<decltype(static_cast<int (T::*)(resmgr_context_t*, io_read_t*,
              RESMGR_OCB_T*, TAttr*)>(&T::read))>> : std::true_type {};

  template <typename T, typename = std::void_t<>>
  struct has_write : std::false_type {};

  template <typename T>
  struct has_write<T,
      std::void_t<decltype(static_cast<int (T::*)(resmgr_context_t*,
              io_write_t*, RESMGR_OCB_T*, TAttr*)>(&T::write))>>
      : std::true_type {};

  template <typename T, typename = std::void_t<>>
  struct has_lseek : std::false_type {};

  template <typename T>
  struct has_lseek<T,
      std::void_t<decltype(static_cast<int (T::*)(resmgr_context_t*,
              io_lseek_t*, RESMGR_OCB_T*, TAttr*)>(&T::lseek))>>
      : std::true_type {};

  template <typename T, typename = std::void_t<>>
  struct has_close_ocb : std::false_type {};

  template <typename T>
  struct has_close_ocb<T,
      std::void_t<decltype(static_cast<int (T::*)(resmgr_context_t*, void*,
              RESMGR_OCB_T*, TAttr*)>(&T::close_ocb))>> : std::true_type {};

  template <typename T, typename = std::void_t<>>
  struct has_close_dup : std::false_type {};

  template <typename T>
  struct has_close_dup<T,
      std::void_t<decltype(static_cast<int (T::*)(resmgr_context_t*,
              io_close_t*, RESMGR_OCB_T*, TAttr*)>(&T::close_dup))>>
      : std::true_type {};

 public:
  /// @brief Default constructor.
  template <typename... Args>
  device(Args&&... args) : driver_{TDriver(std::forward<Args>(args)...)} {}

  // Having copies of a resource manager doesn't make sense. So don't allow it.

  device(const device&) = delete;
  auto operator=(const device&) -> device& = delete;

  // Not movable. Moving while the resource manager is running will result in
  // the datastructures changing their location in memory, which would result in
  // undefined behaviour. So don't allow it.

  device(device&&) = delete;
  auto operator=(device&&) -> device& = delete;

  /// @brief free resources from this class
  ///
  /// Do not call to destroy the object, unless you've already exited the run()
  /// loop. Destroying the class while running results in undefined behaviour
  /// (the class memory is cleaned up while the resource manager loop is
  /// running).
  ~device() = default;

  /// @brief Run the resource manager.
  ///
  /// This function is not thread-safe. Only call it once from the thread that
  /// will run the resource manager.
  ///
  /// @return The code that was given to exit(), or an error code for the
  /// failure. Exit codes are negated, so that a user can use positive integers
  /// in the exit() function to indicate other exit reasons.
  auto run() -> stdext::expected<int, int> {
    if (device_.is_exited()) {
      device_.wait_set();
      return device_.exit_code();
    }
    if (!driver_) {
      device_.wait_set();
      return stdext::unexpected{-EDRVNOTREADY};
    }

    const auto config = driver_.get_config();
    const auto paths = driver_.get_devices();

    resmgr_connect_funcs_t connect_funcs{};
    resmgr_io_funcs_t io_funcs{};
    iofunc_func_init(                            // initialise:
        _RESMGR_CONNECT_NFUNCS, &connect_funcs,  // - connection functions
        _RESMGR_IO_NFUNCS, &io_funcs);           // - I/O functions

    if constexpr (has_open<TDriver>::value) {
      connect_funcs.open = details::resmgr_connect_handler<TDriver, io_open_t,
          void, TAttr, &TDriver::open>;
    }
    if constexpr (has_devctl<TDriver>::value) {
      io_funcs.devctl = details::resmgr_iofunc_handler<TDriver, io_devctl_t,
          TAttr, &TDriver::devctl>;
    }
    if constexpr (has_read<TDriver>::value) {
      io_funcs.read = details::resmgr_iofunc_handler<TDriver, io_read_t, TAttr,
          &TDriver::read>;
      io_funcs.read64 = details::resmgr_iofunc_handler<TDriver, io_read_t,
          TAttr, &TDriver::read>;
    }
    if constexpr (has_write<TDriver>::value) {
      io_funcs.write = details::resmgr_iofunc_handler<TDriver, io_write_t,
          TAttr, &TDriver::write>;
      io_funcs.write64 = details::resmgr_iofunc_handler<TDriver, io_write_t,
          TAttr, &TDriver::write>;
    }
    if constexpr (has_lseek<TDriver>::value) {
      io_funcs.lseek = details::resmgr_iofunc_handler<TDriver, io_lseek_t,
          TAttr, &TDriver::lseek>;
    }
    if constexpr (has_close_ocb<TDriver>::value) {
      io_funcs.close_ocb = details::resmgr_iofunc_handler<TDriver, void, TAttr,
          &TDriver::close_ocb>;
    }
    if constexpr (has_close_dup<TDriver>::value) {
      io_funcs.close_dup = details::resmgr_iofunc_handler<TDriver, io_close_t,
          TAttr, &TDriver::close_dup>;
    }

    return device_.run(driver_, config, paths, connect_funcs, io_funcs);
  };

  /// @brief Wait for the resource manager to start running.
  auto wait() -> void { device_.wait(); }

  /// @brief Wait for the resource manager to start running.
  ///
  /// If the resource manager isn't running in time, then return false.
  ///
  /// @tparam Rep an arithmetic type, or a class emulating an arithmetic type,
  /// representing the number of ticks.
  ///
  /// @tparam Period a std::ratio representing the tick period (i.e. the number
  /// of second's fractions per tick).
  ///
  /// @return true if the driver started in time.
  template <class Rep, class Period>
  auto wait_for(const std::chrono::duration<Rep, Period>& rel_time) -> bool {
    return device_.wait_for(rel_time);
  }

  /// @brief Requests to unblock and stop the resource manager.
  ///
  /// Tries to unblock the resource manager by sending it a pulse. It may not
  /// succeed. If the resource manager is not running, or is not initialised,
  /// nothing will happen.
  ///
  /// Always updates the exit code to the last value. If called multiple times,
  /// the run() method may return (based on race conditions) any value given to
  /// this function.
  ///
  /// @param code The exit code to return from the run() method.
  auto request_exit(int code) noexcept -> void { device_.exit(code); }

  /// @brief Test if the run() method is executing.
  ///
  /// Checks if the resource manager is running. There is the possibility of
  /// slight race conditions when testing if the resource manager is running. If
  /// the method run() is called, and immediately tested for, the internal
  /// datastructures may not have been initialised yet.
  ///
  /// There is a small period of time after a resource manager is currently
  /// exiting, when this operator may still return true.
  ///
  /// The only sure way you can know if the resource manager has exited or not
  /// is through the thread that called the run() method, when the run() method
  /// has exited.
  ///
  /// @return true of the resource manager is running.
  operator bool() const noexcept { return device_.is_running(); }

  /// @brief Get the underlying driver class to access state it may offer.
  ///
  /// @return the underlying driver class.
  auto driver() noexcept -> TDriver& { return driver_; }

 private:
  details::device device_{};
  TDriver driver_;
};

}  // namespace os::qnx::resmgr

// The following are put in the global namespace, to mimic QNX like functions.

using os::qnx::resmgr::as_pread_t;
using os::qnx::resmgr::as_pwrite_t;
using os::qnx::resmgr::devctl_data_cast;
using os::qnx::resmgr::iofunc_devctl_verify_length;
using os::qnx::resmgr::iofunc_read_verify_length;
using os::qnx::resmgr::iofunc_write_verify_length;

#endif
