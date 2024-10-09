#include <cerrno>

#include "ubench/thread.h"

namespace ubench::thread {

auto pin_core([[maybe_unused]] unsigned int core) -> bool {
  errno = ENOSYS;
  return false;
}

}  // namespace ubench::thread
