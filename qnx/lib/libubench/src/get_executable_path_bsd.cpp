#include <sys/types.h>
#include <sys/sysctl.h>
#include <unistd.h>

#include <array>

#include "ubench/os.h"
#include "get_executable_path_common.h"

namespace ubench::os {

auto get_executable_path() -> std::filesystem::path {
  std::array<char, 4096> buffer{};

#if defined(__NetBSD__)
  int mib[4] = {CTL_KERN, KERN_PROC_ARGS, -1, KERN_PROC_PATHNAME};
#else
  int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
#endif

  size_t size = buffer.size();
  auto result = sysctl(mib, 4, buffer.data(), &size, nullptr, 0);

  // Error occurred.
  if (result == -1) return {};

  // Insufficient buffer size.
  if (size == buffer.size()) return {};

  return get_directory_path(buffer.data());
}

}  // namespace ubench::os
