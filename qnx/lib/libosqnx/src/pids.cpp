#include "osqnx/pids.h"

#include <sys/procfs.h>
#include <sys/types.h>
#include <devctl.h>
#include <unistd.h>

#include <filesystem>

#include "ubench/os.h"
#include "ubench/string.h"

namespace {
auto get_filename(std::string path) -> std::string {
  std::filesystem::path p{std::move(path)};
  if (p.has_filename()) {
    return p.filename().string();
  } else {
    return p.string();
  }
}
}  // namespace

namespace os::qnx {

auto pids::get_name(unsigned int pid, bool short_path)
    -> stdext::expected<std::string, int> {
  if (auto search = pid_names_.find(pid); search != pid_names_.end()) {
    return short_path ? get_filename(search->second) : search->second;
  }

  auto proc_name = ubench::os::get_proc_name(pid);
  if (!proc_name) return stdext::unexpected{proc_name.error()};

  auto [it, ins] = pid_names_.try_emplace(pid, *proc_name);
  return short_path ? get_filename(it->second) : it->second;
}

auto pids::drop(unsigned int pid) -> bool { return pid_names_.erase(pid) != 0; }

auto pids::query_pids() -> std::vector<unsigned int> {
  const std::filesystem::path proc{"/proc"};

  std::filesystem::directory_iterator dir_it;
  try {
    dir_it = std::filesystem::directory_iterator{proc};
  } catch (std::filesystem::filesystem_error& e) {
    return {};
  }

  std::vector<unsigned int> pids{};
  for (auto const& pid_entry : dir_it) {
    try {
      if (pid_entry.is_directory()) {
        auto pid = ubench::string::parse_int<unsigned int>(
            pid_entry.path().filename().string());
        if (pid) {
          pids.push_back(*pid);
        }
      }
    } catch (std::filesystem::filesystem_error& e) {
      // Ignore the error that the directory can't be read.
    }
  }
  return pids;
}

}  // namespace os::qnx
