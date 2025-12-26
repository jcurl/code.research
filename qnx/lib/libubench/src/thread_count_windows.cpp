#include <windows.h>

namespace ubench::thread {

[[nodiscard]] auto thread_count() -> unsigned int {
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
}

}  // namespace ubench::thread
