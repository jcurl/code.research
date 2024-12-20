#include "get_proc_name_common.h"

#include <fstream>
#include <sstream>

namespace ubench::os {

auto get_name_cmdline(pid_t pid) -> std::optional<std::string> {
  std::ostringstream pathstream{};
  pathstream << "/proc/" << pid << "/cmdline";

  std::ifstream file(pathstream.str(), std::ios::binary);
  if (!file.is_open()) return {};

  std::string procname{};
  std::getline(file, procname, '\0');
  return procname;
}

}  // namespace ubench::os
