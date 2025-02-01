#include "config.h"

#include "mlock.h"

#include <cerrno>

#if HAVE_MLOCKALL
#include <sys/mman.h>
#endif

auto enable_mlockall() -> stdext::expected<void, int> {
#if HAVE_MLOCKALL
  if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
    return stdext::unexpected{errno};
  }
#endif
  return {};
}
