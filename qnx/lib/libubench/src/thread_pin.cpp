#include "config.h"

#include <pthread.h>

#include "stdext/expected.h"
#if HAVE_INCLUDE_PTHREAD_NP_H
// FreeBSD
#include <pthread_np.h>
#endif

#include <sched.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "ubench/thread.h"

namespace ubench::thread {

auto pin_core(unsigned int core) -> stdext::expected<void, int> {
  if (core >= std::thread::hardware_concurrency())
    return stdext::unexpected{EINVAL};

  pthread_t current = pthread_self();

#if defined(__NetBSD__)
  errno = 0;
  cpuset_t *cpuset = cpuset_create();
  if (cpuset == nullptr) return stdext::unexpected{errno};

  cpuid_t ci = core;
  cpuset_set(ci, cpuset);
  int rc = pthread_setaffinity_np(current, cpuset_size(cpuset), cpuset);
  cpuset_destroy(cpuset);
#elif defined(__linux__) || defined(__CYGWIN__) || defined(__FreeBSD__)
  // Linux, Cygwin
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  int rc = pthread_setaffinity_np(current, sizeof(cpu_set_t), &cpuset);
#else
  // Unknown. A port is needed.
  return stdext::unexpected{ENOSYS};
#endif
  if (rc != 0) return stdext::unexpected{rc};
  return {};
}

}  // namespace ubench::thread
