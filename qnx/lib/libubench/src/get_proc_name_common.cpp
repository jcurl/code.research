#include "get_proc_name_common.h"

#include <cerrno>
#include <fstream>
#include <sstream>

namespace ubench::os {

auto get_name_cmdline(pid_t pid) -> stdext::expected<std::string, int> {
  std::ostringstream pathstream{};
  pathstream << "/proc/" << pid << "/cmdline";

  errno = 0;  // Ensure the error code is reset to avoid undefined behaviour.
  std::ifstream file(pathstream.str(), std::ios::binary);
  if (!file.is_open()) {
    if (file.fail()) return stdext::unexpected{errno};
    return stdext::unexpected{EINVAL};
  }

  std::string procname{};
  std::getline(file, procname, '\0');
  return procname;
}

}  // namespace ubench::os
