#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ubench/os.h"
#include "get_executable_path_common.h"

namespace ubench::os {

auto get_executable_path() -> std::filesystem::path {
  std::array<char, 4096> buffer{};

  // Platform API call.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  int fd = open("/proc/self/exefile", O_RDONLY);
  if (fd == -1) return {};

  ssize_t bytes_read = read(fd, buffer.data(), buffer.size());
  close(fd);

  // Error occurred, or empty string.
  if (bytes_read <= 0) return {};

  // Insufficient buffer size. If full and has a NUL character, then it did fit.
  // QNX usually has a NUL terminator at the end.
  if (bytes_read == buffer.size() && buffer[buffer.size() - 1] != 0) return {};

  return get_directory_path(buffer.data());
}

}  // namespace ubench::os
