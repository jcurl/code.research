#include <sys/neutrino.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "ubench/thread.h"

namespace ubench::thread {

auto pin_core(unsigned int core) -> bool {
  if (core >= std::thread::hardware_concurrency()) {
    errno = EINVAL;
    return false;
  }

  unsigned int runmask = 1 << core;
  int result = ThreadCtl(_NTO_TCTL_RUNMASK_GET_AND_SET, &runmask);
  return result != -1;
}

}  // namespace ubench::thread
