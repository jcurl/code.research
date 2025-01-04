#include <unistd.h>

#include "ubench/os.h"

namespace ubench::os {

auto get_proc_name([[maybe_unused]] pid_t pid) -> std::optional<std::string> {
  return {};
}

}  // namespace ubench::os
