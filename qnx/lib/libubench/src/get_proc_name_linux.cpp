#include <unistd.h>

#include "ubench/os.h"
#include "get_proc_name_common.h"

namespace ubench::os {

auto get_proc_name(pid_t pid) -> std::optional<std::string> {
  return get_name_cmdline(pid);
}

}  // namespace ubench::os