#include <sys/types.h>
#include <sys/user.h>
#include <libutil.h>

#include <cerrno>

#include "ubench/os.h"

namespace ubench::os {

auto get_proc_name(pid_t pid) -> stdext::expected<std::string, int> {
  kinfo_proc *proc = kinfo_getproc(pid);
  if (!proc) return stdext::unexpected{errno};

  std::string path{&proc->ki_comm[0]};
  free(proc);
  return path;
}

}  // namespace ubench::os
