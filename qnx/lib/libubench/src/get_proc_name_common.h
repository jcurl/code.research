#ifndef UBENCH_OS_GET_PROC_NAME_COMMON_H
#define UBENCH_OS_GET_PROC_NAME_COMMON_H

#include <unistd.h>

#include <string>

#include "stdext/expected.h"

namespace ubench::os {

auto get_name_cmdline(pid_t pid) -> stdext::expected<std::string, int>;

}  // namespace ubench::os

#endif
