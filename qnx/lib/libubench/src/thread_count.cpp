#include <thread>

namespace ubench::thread {

[[nodiscard]] auto thread_count() -> unsigned int {
  return std::thread::hardware_concurrency();
}

}  // namespace ubench::thread
