#include <unistd.h>

#include <thread>

namespace ubench::thread {

[[nodiscard]] auto thread_count() -> unsigned int {
  long nproc = sysconf(_SC_NPROCESSORS_CONF);
  if (nproc < 0) {
    return std::thread::hardware_concurrency();
  }
  return static_cast<unsigned int>(nproc);
}

}  // namespace ubench::thread
