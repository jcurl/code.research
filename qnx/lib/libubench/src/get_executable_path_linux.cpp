#include <unistd.h>

#include <array>

#include "ubench/os.h"
#include "get_executable_path_common.h"

namespace ubench::os {

auto get_executable_path() -> std::filesystem::path {
  std::array<char, 4096> buffer{};
  ssize_t len = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);

  // Error occurred.
  if (len == -1) return {};

  // Insufficient buffer size.
  if (len == buffer.size() - 1) return {};

  return get_directory_path(buffer.data());
}

}  // namespace ubench::os
