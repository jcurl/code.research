#include <sys/procfs.h>
#include <sys/types.h>
#include <devctl.h>
#include <unistd.h>

#include <sstream>
#include <string>

#include "ubench/file.h"
#include "ubench/os.h"
#include "get_proc_name_common.h"

namespace {

auto get_name_as(pid_t pid) -> stdext::expected<std::string, int> {
  std::ostringstream pathstream{};
  pathstream << "/proc/" << pid << "/as";
  ubench::file::fdesc fd{pathstream.str()};
  if (!fd) return stdext::unexpected{errno};

  using procfs_debuginfo_tx = ubench::os::osbuff<procfs_debuginfo, 1024>;
  procfs_debuginfo_tx map{};

  // Gets the base address of the binary. This is unsafe.
  int status =
      devctl(fd, DCMD_PROC_MAPDEBUG_BASE, &map(), sizeof(map), nullptr);
  if (status != EOK) return stdext::unexpected{status};

  return std::string{map().path};
}

}  // namespace

namespace ubench::os {

auto get_proc_name(pid_t pid) -> stdext::expected<std::string, int> {
  auto procname = get_name_as(pid);
  if (procname) return procname;

  return get_name_cmdline(pid);
}

}  // namespace ubench::os
