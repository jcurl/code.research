#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>

#include <cerrno>

#include "ubench/os.h"

namespace ubench::os {

auto get_proc_name(pid_t pid) -> stdext::expected<std::string, int> {
  std::array<int, 4> mib{CTL_KERN, KERN_PROC, KERN_PROC_PID, pid};
  kinfo_proc proc{};
  size_t size = sizeof(kinfo_proc);
  int st = sysctl(mib.data(), mib.size(), &proc, &size, nullptr, 0);
  if (st == -1) {
    printf("sysctl errno=%d", errno);
    return stdext::unexpected{errno};
  }

  std::string path{&proc.ki_comm[0]};
  return path;
}

}  // namespace ubench::os
