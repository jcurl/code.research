#include "thread_pin.h"

#include <pthread.h>

#include <cstdio>
#include <cstdlib>
#include <thread>

auto thread_pin_core(unsigned int core) -> void {
  if (core >= std::thread::hardware_concurrency()) std::abort();

  pthread_t current = pthread_self();

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  int rc = pthread_setaffinity_np(current, sizeof(cpu_set_t), &cpuset);
  if (rc) {
    perror("thread_pin_core");
    std::abort();
  }
}
