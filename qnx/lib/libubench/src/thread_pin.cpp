#include <pthread.h>
#include <sched.h>

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

#if defined(__NetBSD__)
  cpuset_t *cpuset = cpuset_create();
  if (cpuset == nullptr) return false;

  cpuid_t ci = core;
  cpuset_set(ci, cpuset);
  int rc = pthread_setaffinity_np(current, cpuset_size(cpuset), cpuset);
  cpuset_destroy(cpuset);
#elif defined(__linux__) || defined(__CYGWIN__)
  // Linux, Cygwin
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  int rc = pthread_setaffinity_np(current, sizeof(cpu_set_t), &cpuset);
#else
  // Unknown. A port is needed.
  int rc = -1;
  errno = ENOSYS;
#endif
  return rc == 0;
}

}  // namespace ubench::thread
