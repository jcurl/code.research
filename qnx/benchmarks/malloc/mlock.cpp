#include "config.h"

#include "mlock.h"

#include <cerrno>
#include <cstring>

#if HAVE_MLOCKALL
#include <sys/mman.h>
#endif

auto enable_mlockall() -> std::optional<std::string_view> {
#if HAVE_MLOCKALL
  if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
    return strerror(errno);
  }
#endif
  return {};
}
