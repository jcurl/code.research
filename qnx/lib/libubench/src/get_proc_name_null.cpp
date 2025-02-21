#include <unistd.h>

#include <cerrno>

#include "ubench/os.h"

namespace ubench::os {

auto get_proc_name([[maybe_unused]] pid_t pid)
    -> stdext::expected<std::string, int> {
  return stdext::unexpected{ENOSYS};
}

}  // namespace ubench::os
