#include <vector>

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
  cpuid_driver() { init_cpuid_driver(); }

  /// @brief Resource manager configuration that os::qnx::resmgr::device uses.
  /// @return The configuration
  [[nodiscard]] auto get_config() const noexcept
      -> const os::qnx::resmgr::config& {
    return config_;
  }

  [[nodiscard]] auto get_devices() const noexcept
      -> const std::vector<os::qnx::resmgr::rsrc<attr_type>>& {
    return devices_;
  }

  auto read(resmgr_context_t*, io_read_t* msg, RESMGR_OCB_T* ocb,
      [[maybe_unused]] attr_type* attr) noexcept -> int;

  auto lseek(resmgr_context_t*, io_lseek_t* msg, RESMGR_OCB_T* ocb,
      [[maybe_unused]] attr_type* attr) noexcept -> int;

 private:
  os::qnx::resmgr::config config_{
      .self = false, .fixed_prio = false, .inherit_runmask = false};
  std::vector<attr_type> attrs_;
  std::vector<os::qnx::resmgr::rsrc<attr_type>> devices_;

  // Sets up the vector devices_ based on the number of CPUs.
  auto init_cpuid_driver() -> void;
};
