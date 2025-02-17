#include <sys/param.h>
#include <sys/sysctl.h>
#include <fcntl.h>
#include <kvm.h>

#include <cerrno>

#include "stdext/expected.h"
#include "ubench/os.h"

namespace ubench::os {

auto get_proc_name(pid_t pid) -> stdext::expected<std::string, int> {
  std::array<int, 6> mib{
      CTL_KERN, KERN_PROC2, KERN_PROC_PID, pid, sizeof(kinfo_proc2), 0};

  size_t size{};
  int st = sysctl(mib.data(), mib.size(), nullptr, &size, nullptr, 0);
  if (st == -1) return stdext::unexpected{errno};

  kinfo_proc2 proc{};
  mib[5] = (size / sizeof(kinfo_proc2));
  st = sysctl(mib.data(), mib.size(), &proc, &size, nullptr, 0);
  if (st == -1) return stdext::unexpected{errno};

  std::string path{&proc.p_comm[0]};
  return path;
}

}  // namespace ubench::os
