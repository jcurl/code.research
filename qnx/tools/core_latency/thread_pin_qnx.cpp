#include <sys/neutrino.h>

#include <cstdio>
#include <cstdlib>

#include "thread_pin.h"

auto thread_pin_core(unsigned int core) -> void {
  unsigned int runmask = 1 << core;
  int result = ThreadCtl(_NTO_TCTL_RUNMASK_GET_AND_SET, &runmask);
  if (result == -1) {
    perror("thread_pin_core");
    std::abort();
  }
}
