#ifndef UBENCH_OS_GET_PROC_NAME_COMMON_H
#define UBENCH_OS_GET_PROC_NAME_COMMON_H

#include <unistd.h>

#include <optional>
#include <string>

namespace ubench::os {

auto get_name_cmdline(pid_t pid) -> std::optional<std::string>;

}  // namespace ubench::os

#endif
