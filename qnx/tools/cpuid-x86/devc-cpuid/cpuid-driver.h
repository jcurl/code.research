#ifndef CPUID_DEVC_CPUID_DRIVER_H
#define CPUID_DEVC_CPUID_DRIVER_H

#include <vector>

#include "cpuid/cpuid.h"
#include "cpuid/cpuidreader_cache.h"
#include "cpuid/cpuidreader_native.h"
#include "osqnx/resmgr/device.h"

/// @brief Per device path attribute structure
///
/// Enables the cpuid_device to know which path the callback is being used for.
struct cpuid_attribute {
  unsigned int core;  //< The thread number to pin when executing CPUID.
};

/// @brief Defines the device driver for the CPUID instruction.
class cpuid_driver {
 public:
  using attr_type = cpuid_attribute;
  operator bool() const noexcept { return devices_.size() > 0; }
  cpuid_driver() = default;

  /// @brief Resource manager configuration that os::qnx::resmgr::device uses.
  /// @return The configuration
  [[nodiscard]] auto get_config() const noexcept
      -> const os::qnx::resmgr::config& {
    return config_;
  }

  /// @brief Get the list of devices, valid after init() is called.
  /// @return The list of paths and their description to register as a resource
  /// manager.
  [[nodiscard]] auto get_devices() const noexcept
      -> const std::vector<os::qnx::resmgr::rsrc<attr_type>>& {
    return devices_;
  }

  /// @brief Initialise the cpuid_driver.
  ///
  /// This occurs when the driver is first run, called by the driver class. This
  /// allows the user to programmatically set the options, without it having to
  /// be done in the constructor.
  auto init() -> void;

  /// @brief Set the user identifier that should be used for the device.
  /// @param user The user identifier.
  auto set_user(uid_t user) noexcept -> void { user_ = user; }

  /// @brief Set the group identifier that should be used for the device.
  /// @param group The group identifier.
  auto set_group(gid_t group) noexcept -> void { group_ = group; }

  /// @brief Set the permissions (masked to 0777).
  /// @param permissions The permissions to set.
  auto set_permissions(unsigned int permissions) noexcept -> void {
    perms_ = permissions;
  }

  /// @brief The QNX io_read callback function.
  /// @param ctp The resource context.
  /// @param msg The io_read message.
  /// @param ocb The open control block.
  /// @param attr Attributes for this path.
  /// @return The return code to the qnx resource manager libc implementation.
  auto read(resmgr_context_t* ctp, io_read_t* msg, RESMGR_OCB_T* ocb,
      attr_type* attr) noexcept -> int;

 private:
  os::qnx::resmgr::config config_{
      .self = false, .fixed_prio = false, .inherit_runmask = false};
  std::vector<attr_type> attrs_;
  std::vector<os::qnx::resmgr::rsrc<attr_type>> devices_;
  rjcp::cpuid::cpuidreader_native cpuid_{};
  rjcp::cpuid::cpuidreader_cache cpuid_cache_{cpuid_};
  uid_t user_;
  gid_t group_;
  unsigned int perms_;
};

#endif
