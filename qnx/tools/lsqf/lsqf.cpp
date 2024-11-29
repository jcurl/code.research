#include <sys/iomsg.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <sys/procfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <charconv>
#include <climits>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <vector>

#include "ubench/args.h"
#include "ubench/file.h"
#include "ubench/os.h"

auto get_process_name(int pid) -> std::string {
  std::ostringstream pathstream{};
  pathstream << "/proc/" << pid << "/as";
  ubench::file::fdesc fd{pathstream.str()};
  if (!fd) return {};

  using procfs_debuginfo_tx = ubench::os::osbuff<procfs_debuginfo, 1024>;
  procfs_debuginfo_tx map{};
  // Gets the base address of the binary. This is unsafe.
  devctl(fd, DCMD_PROC_MAPDEBUG_BASE, &map(), sizeof(map), nullptr);
  return std::string{map().path};
}

auto get_pid_list() -> std::vector<int> {
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

auto print_coids(int pid, bool show_dead) -> void {
  struct _server_info sinfo;

  int fd = 0;
  while (true) {
    // coids are file descriptors. See
    // https://www.qnx.com/developers/docs/7.1/#com.qnx.doc.neutrino.lib_ref/topic/c/connectattach.html
    //
    // Note, this is just an identifier, do NOT close(fd).
    fd = ConnectServerInfo(pid, fd, &sinfo);
    if (fd < 0) return;

    // Don't care about print FDs connected to ourselves.
    if (sinfo.pid != pid) {
      if (show_dead || ((sinfo.flags & _NTO_COF_DEAD) == 0)) {
        // Only care about FDs and not side channels (non-fds).
        if ((fd & _NTO_SIDE_CHANNEL) == 0) {
          std::cout << "fd=" << fd << " connected to "
                    << get_process_name(sinfo.pid) << " (" << sinfo.pid << ")"
                    << std::endl;

          ubench::file::fdesc dup_fd = ConnectAttach(sinfo.nd, sinfo.pid,
              sinfo.chid, 0, _NTO_COF_CLOEXEC | _NTO_COF_NOEVENT);
          if (dup_fd) {
            io_dup_t msg{};
            msg.i.type = _IO_DUP;
            msg.i.combine_len = sizeof(msg);
            msg.i.info.nd = netmgr_remote_nd(sinfo.nd, ND_LOCAL_NODE);
            msg.i.info.pid = pid;
            msg.i.info.chid = sinfo.chid;
            msg.i.info.scoid = sinfo.scoid;
            msg.i.info.coid = fd;

            // We can't affort to block for long, .5 a second is all we will
            // tolerate. At the very least, when this happens we should still be
            // able to log the entry, just not with any name information.
            {
              struct sigevent event;
              memset(&event, 0, sizeof(event));
              event.sigev_notify = SIGEV_UNBLOCK;
              std::uint64_t nsec = 1 * 500000000L;

              TimerTimeout(CLOCK_MONOTONIC,
                  _NTO_TIMEOUT_SEND | _NTO_TIMEOUT_REPLY, &event, &nsec, NULL);
              if (MsgSendnc(dup_fd, &msg.i, sizeof(msg.i), 0, 0) == -1) {
                ConnectDetach(dup_fd.reset());
              } else {
                struct _fdinfo fdinfo {};

                // OS integration
                //
                // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
                char path[PATH_MAX];
                path[0] = 0;

                if (iofdinfo(dup_fd, _FDINFO_FLAG_LOCALPATH, &fdinfo, path,
                        sizeof(path)) >= 0) {
                  std::cout << " path=" << path << "; mode=" << fdinfo.mode
                            << "; ioflag=" << fdinfo.ioflag
                            << "; size=" << fdinfo.size
                            << "; offset=" << fdinfo.offset
                            << "; flags=" << fdinfo.flags
                            << "; share=" << fdinfo.sflag
                            << "; count=" << fdinfo.count
                            << "; rcount=" << fdinfo.rcount
                            << "; wcount=" << fdinfo.wcount
                            << "; rlocks=" << fdinfo.rlocks
                            << "; wlocks=" << fdinfo.wlocks << std::endl;
                }
              }
            }
          }
        }
      }
    }
    fd++;
  }
}

auto main(int argc, char* argv[]) -> int {
  std::vector<int> pids{};

  int c = 0;
  bool help = false;
  int exit_code = 0;

  while ((c = getopt(argc, argv, "p:?")) != -1) {
    switch (c) {
      case 'p': {
        auto arglist = ubench::args::split_args(optarg);
        if (!arglist) {
          std::cerr << "Error: No arguments provided for pid list" << std::endl;
          exit_code = 1;
          break;
        }
        for (const auto& arg : arglist.value()) {
          auto pid_arg = ubench::args::parse_int<unsigned int>(arg);
          if (pid_arg) {
            pids.push_back(*pid_arg);
          } else {
            std::cerr << "Error: PID " << arg << " invalid format" << std::endl;
            exit_code = 1;
            break;
          }
        }
        break;
      }
      case '?':
        help = true;
        if (optopt) exit_code = 1;
        break;
      case ':':
        std::cerr << "Error: Option -" << optopt << " requires an operand"
                  << std::endl;
        exit_code = 1;
        help = true;
        break;
      default:
        std::cerr << "Error: Unknown option -" << optopt << std::endl;
        exit_code = 1;
        help = true;
        break;
    }
  }

  if (help || exit_code) {
    // TODO: Print help
    return exit_code;
  }

  if (pids.size() == 0) {
    pids = get_pid_list();
  }

  for (auto const& pid : pids) {
    print_coids(pid, false);
  }
}