#include <pthread.h>

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

  pthread_t current = pthread_self();

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  int rc = pthread_setaffinity_np(current, sizeof(cpu_set_t), &cpuset);
  return rc == 0;
}

}  // namespace ubench::thread
