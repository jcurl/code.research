#include <fstream>
#include <string>
#include <thread>

namespace ubench::thread {

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static unsigned int core_count_{0};

auto init_core_count() -> bool {
  // This function should only be called by init_static(), so it is only
  // called once.

  // Can't use sysconf(_SC_NPROCESSORS_CONF), as Alpine musl returns the number
  // of pinned threads, not the total number of CPUs. Same for
  // get_nprocs_conf().

  std::ifstream cpuinfo("/proc/cpuinfo");
  if (!cpuinfo.is_open()) return false;

  std::string line;
  unsigned int core_count = 0;

  // Read through each line of the file
  while (std::getline(cpuinfo, line)) {
    // Look for lines starting with "processor" (each CPU is listed as a
    // processor entry)
    if (line.find("processor") == 0) {
      core_count++;
    }
  }

  cpuinfo.close();

  core_count_ = core_count;
  return true;
}

auto init_static() -> bool {
  static const auto result = init_core_count();
  return result;
}

}  // namespace

[[nodiscard]] auto thread_count() -> unsigned int {
  if (!init_static()) return std::thread::hardware_concurrency();
  return core_count_;
}

}  // namespace ubench::thread
