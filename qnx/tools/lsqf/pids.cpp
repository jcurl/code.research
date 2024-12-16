#include "pids.h"

#include <sys/procfs.h>
#include <sys/types.h>
#include <devctl.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "ubench/args.h"
#include "ubench/file.h"
#include "ubench/os.h"

namespace {
auto get_filename(std::string path) -> std::string {
  std::filesystem::path p{std::move(path)};
  if (p.has_filename()) {
    return p.filename().string();
  } else {
    return p.string();
  }
}

auto get_name_as(int pid) -> std::optional<std::string> {
  std::ostringstream pathstream{};
  pathstream << "/proc/" << pid << "/as";
  ubench::file::fdesc fd{pathstream.str()};
  if (!fd) return {};

  using procfs_debuginfo_tx = ubench::os::osbuff<procfs_debuginfo, 1024>;
  procfs_debuginfo_tx map{};

  // Gets the base address of the binary. This is unsafe.
  int status =
      devctl(fd, DCMD_PROC_MAPDEBUG_BASE, &map(), sizeof(map), nullptr);
  if (status != EOK) return {};

  return std::string{map().path};
}

auto get_name_cmdline(int pid) -> std::optional<std::string> {
  std::ostringstream pathstream{};
  pathstream << "/proc/" << pid << "/cmdline";

  std::ifstream file(pathstream.str(), std::ios::binary);
  if (!file.is_open()) return {};

  std::string procname{};
  std::getline(file, procname, '\0');
  return procname;
}
}  // namespace

auto pids::get_name(int pid, bool short_path) -> std::optional<std::string> {
  if (auto search = pid_names_.find(pid); search != pid_names_.end()) {
    return short_path ? get_filename(search->second) : search->second;
  }

  auto x = get_name_as(pid);
  if (!x) {
    x = get_name_cmdline(pid);
    if (!x) return {};
  }

  auto [it, ins] = pid_names_.try_emplace(pid, *x);
  return short_path ? get_filename(it->second) : it->second;
}

auto pids::drop(int pid) -> bool { return pid_names_.erase(pid) != 0; }

auto pids::get_pids() -> std::vector<int> {
  const std::filesystem::path proc{"/proc"};

  std::filesystem::directory_iterator dir_it;
  try {
    dir_it = std::filesystem::directory_iterator{proc};
  } catch (std::filesystem::filesystem_error& e) {
    return {};
  }

  std::vector<int> pids{};
  for (auto const& pid_entry : dir_it) {
    try {
      if (pid_entry.is_directory()) {
        auto pid =
            ubench::args::parse_int<int>(pid_entry.path().filename().string());
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