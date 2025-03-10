#include <sys/dcmd_ip.h>
#include <sys/iomsg.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <sys/pathmsg.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>
#include <sstream>

#include "osqnx/pids.h"
#include "ubench/file.h"
#include "file.h"
#include "options.h"

namespace {
os::qnx::pids p{};

class coids {
 public:
  coids() : self_pid_{getpid()} {}
  coids(const coids&) = default;
  auto operator=(const coids&) -> coids& = default;
  coids(coids&&) = default;
  auto operator=(coids&&) -> coids& = default;

  auto show_dead() const& -> const bool& { return show_dead_; }
  auto show_dead() & -> bool& { return show_dead_; }

  auto show_side_channel() const& -> const bool& { return show_side_channel_; }
  auto show_side_channel() & -> bool& { return show_side_channel_; }

  auto show_full_path() const& -> const bool& { return show_full_path_; }
  auto show_full_path() & -> bool& { return show_full_path_; }

  auto show_self() const& -> const bool& { return show_self_; }
  auto show_self() & -> bool& { return show_self_; }

  auto print_hdr() -> void {
    std::ios cstate{nullptr};
    cstate.copyfmt(std::cout);
    std::cout << std::left;
    std::cout << std::setw(25) << "Process (PID)"
              << " " << std::setw(10) << "FD (flags)"
              << " " << std::setw(25) << "RsrcMgr (PID)"
              << " " << std::setw(10) << "Mode"
              << " "
              << "Path" << std::endl;

    std::cout << std::setfill('-');
    std::cout << std::setw(25) << ""
              << " " << std::setw(10) << ""
              << " " << std::setw(25) << ""
              << " " << std::setw(10) << ""
              << " " << std::setw(10) << "" << std::endl;
    std::cout.copyfmt(cstate);
  }

  auto print(int pid) -> void {
    _server_info sinfo{};
    int fd = 0;
    while (true) {
      // coids are file descriptors. See
      // https://www.qnx.com/developers/docs/7.1/#com.qnx.doc.neutrino.lib_ref/topic/c/connectattach.html
      //
      // Note, this is just an identifier, do NOT close(fd).
      fd = ConnectServerInfo(pid, fd, &sinfo);
      if (fd < 0) return;

      print(pid, fd, &sinfo);
      fd++;
    }
  }

 private:
  bool show_self_{false};
  bool show_dead_{false};
  bool show_side_channel_{false};
  bool show_full_path_{false};
  int self_pid_{0};

  auto print(int pid, int fd, _server_info* sinfo) -> void {
    if (!show_self_ && sinfo->pid == pid) return;

    bool is_dead{};
    bool is_side_channel{};
    std::string fd_flags{};

    if ((fd & _NTO_SIDE_CHANNEL) != 0) {
      if (!show_side_channel_) return;
      fd_flags += 's';
      is_side_channel = true;
    }
    if ((sinfo->flags & _NTO_COF_DEAD) != 0) {
      if (!show_dead_) return;
      fd_flags += 'd';
      is_dead = true;
    }
    if ((sinfo->flags & _NTO_COF_CLOEXEC) != 0) {
      fd_flags += 'c';
    }

    struct _fdinfo fdinfo_ {};
    struct _fdinfo* fdinfo = nullptr;
    std::string path{};
    if (!is_dead && !is_side_channel) {
      // OS integration
      //
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
      char ospath[PATH_MAX];
      ospath[0] = 0;

      if (pid != self_pid_) {
        ubench::file::fdesc dup_fd = ConnectAttach(sinfo->nd, sinfo->pid,
            sinfo->chid, 0, _NTO_COF_CLOEXEC | _NTO_COF_NOEVENT);
        if (dup_fd) {
          io_dup_t msg{};
          msg.i.type = _IO_DUP;
          msg.i.combine_len = sizeof(msg);
          msg.i.info.nd = netmgr_remote_nd(sinfo->nd, ND_LOCAL_NODE);
          msg.i.info.pid = pid;
          msg.i.info.chid = sinfo->chid;
          msg.i.info.scoid = sinfo->scoid;
          msg.i.info.coid = fd;

          sigevent event{};
          event.sigev_notify = SIGEV_UNBLOCK;
          std::uint64_t nsec = 1 * 500000000L;

          // Don't block more than 0.5s.
          TimerTimeout(CLOCK_MONOTONIC, _NTO_TIMEOUT_SEND | _NTO_TIMEOUT_REPLY,
              &event, &nsec, NULL);
          if (MsgSendnc(dup_fd, &msg.i, sizeof(msg.i), 0, 0) == -1) {
            // Close the connection explicitly, because the duplicate didn't
            // occur.
            ConnectDetach(dup_fd.reset());
          } else {
            if (iofdinfo(dup_fd, _FDINFO_FLAG_LOCALPATH, &fdinfo_, ospath,
                    sizeof(ospath)) >= 0) {
              fdinfo = &fdinfo_;
              path = std::string{ospath};
              if (S_ISSOCK(fdinfo_.mode)) {
                int status = devctl(
                    dup_fd, DCMD_IP_FDINFO, ospath, sizeof(ospath), nullptr);
                if (status == EOK) path = std::string{ospath};
              }
            }
          }
        }
      } else {
        if (iofdinfo(fd, _FDINFO_FLAG_LOCALPATH, &fdinfo_, ospath,
                sizeof(ospath)) >= 0) {
          fdinfo = &fdinfo_;
          path = std::string{ospath};
          if (S_ISSOCK(fdinfo_.mode)) {
            int status =
                devctl(fd, DCMD_IP_FDINFO, ospath, sizeof(ospath), nullptr);
            if (status == EOK) path = std::string{ospath};
          }
        }
      }
    }

    if (fdinfo) {
      std::uint32_t ioflag{0};
      if (sinfo->pid == PATHMGR_PID && fdinfo->mode == 0) {
        ioflag = fdinfo->flags;
      } else {
        ioflag = fdinfo->ioflag;
      }
      fd_flags += (ioflag & _IO_FLAG_RD) ? 'r' : '-';
      fd_flags += (ioflag & _IO_FLAG_WR) ? 'w' : '-';
    }

    std::ostringstream proc{};
    auto procpid = p.get_name(pid, !show_full_path_);
    if (procpid) {
      proc << *procpid << " (" << pid << ")";
    } else {
      proc << "(" << pid << ")";
    }

    std::ostringstream fdc{};
    fdc << (fd & ~_NTO_SIDE_CHANNEL) << fd_flags;

    std::ostringstream conn{};
    auto rsrcpid = p.get_name(sinfo->pid, !show_full_path_);
    if (rsrcpid) conn << *rsrcpid;

    if (show_self_ && sinfo->pid == pid) {
      int selffd = sinfo->scoid & ~_NTO_CONNECTION_SCOID;
      if (selffd & _NTO_SIDE_CHANNEL) {
        conn << ":" << (selffd & ~_NTO_SIDE_CHANNEL) << "s";
      } else {
        conn << ":" << selffd;
      }
    }
    conn << " (" << sinfo->pid << ")";

    std::cout << std::left << std::setw(25) << proc.str() << " ";
    std::cout << std::left << std::setw(10) << fdc.str() << " ";
    std::cout << std::left << std::setw(25) << conn.str() << " ";

    if (fdinfo) {
      std::cout << std::setw(10) << modestr(fdinfo->mode) << " " << path;
    }

    std::cout << std::endl;
  }
};

}  // namespace

auto main(int argc, char* argv[]) -> int {
  coids lsqf{};
  std::vector<unsigned int> pids{};

  auto options = make_options(argc, argv);
  if (!options) return options.error();
  lsqf.show_side_channel() = options->show_sidechannels();
  lsqf.show_dead() = options->show_dead();
  lsqf.show_self() = options->show_self();
  if (options->verbosity() >= 1) {
    lsqf.show_full_path() = true;
  }

  if (options->pids().empty()) {
    pids = p.query_pids();
  } else {
    pids = options->pids();
  }

  lsqf.print_hdr();
  for (auto const& pid : pids) {
    lsqf.print(pid);
  }
}
