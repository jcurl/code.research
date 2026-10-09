#ifndef CPUID_TEST_CPUID_DEV_H
#define CPUID_TEST_CPUID_DEV_H

#include "ubench/file.h"

/// @brief Checks if the system has installed the cpuid kernel driver.
/// @return true if the driver is available.
auto has_cpuid_dev_driver() -> bool;

/// @brief Gets a file descriptor for the CPUID kernel driver for the core
/// specified.
/// @param cpunum The core to open the device for.
/// @return The file descriptor, which is invalid if the device doesn't exist.
auto get_cpuid_dev_fdpath(unsigned int cpunum) -> ubench::file::fdesc;

#endif
