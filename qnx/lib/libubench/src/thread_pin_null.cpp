#include <cerrno>

#include "stdext/expected.h"
#include "ubench/thread.h"

namespace ubench::thread {

auto pin_core([[maybe_unused]] unsigned int core)
    -> stdext::expected<void, int> {
  return stdext::unexpected{ENOSYS};
}

}  // namespace ubench::thread
