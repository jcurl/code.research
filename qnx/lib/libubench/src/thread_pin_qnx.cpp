#include <sys/neutrino.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "stdext/expected.h"
#include "ubench/thread.h"

namespace ubench::thread {

auto pin_core(unsigned int core) -> stdext::expected<void, int> {
  if (core >= std::thread::hardware_concurrency())
    return stdext::unexpected{EINVAL};

  unsigned int runmask = 1 << core;
  int result = ThreadCtl(_NTO_TCTL_RUNMASK_GET_AND_SET, &runmask);
  if (result == -1) return stdext::unexpected{errno};
  return {};
}

}  // namespace ubench::thread
