#include "osqnx/resmgr/device.h"

namespace os::qnx::resmgr {

namespace details {

auto device::run_internal(void *driver, const config &config,
    const std::vector<rsrc<>> *paths, const resmgr_connect_funcs_t *connect,
    const resmgr_io_funcs_t *io) noexcept -> stdext::expected<int, int> {
  if (dpp_) return stdext::unexpected{-ERUNNING};
  if (paths->empty()) {
    wait_set();
    return stdext::unexpected{-EDRVNOPATH};
  }

  int flags = _NTO_CHF_UNBLOCK | _NTO_CHF_DISCONNECT;
  if (config.fixed_prio) flags |= _NTO_CHF_FIXED_PRIORITY;
  if (config.inherit_runmask) flags |= _NTO_CHF_INHERIT_RUNMASK;
  int chid = ChannelCreate(flags);
  if (chid == -1) {
    wait_set();
    return stdext::unexpected{-errno};
  }

  dpp_ = dispatch_create_channel(chid, DISPATCH_FLAG_NOLOCK);
  if (dpp_ == nullptr) {
    ChannelDestroy(chid);
    wait_set();
    return stdext::unexpected{-errno};
  }

  resmgr_attr_t resmgr_attr{};
  resmgr_attr.nparts_max = 1;
  resmgr_attr.msg_max_size = 2048;

  const auto path_count = paths->size();
  std::vector<iofunc_attr_ext_t> attr(path_count);
  std::vector<int> ids(path_count);
  for (std::size_t i = 0; i < path_count; i++) {
    iofunc_attr_init(reinterpret_cast<iofunc_attr_t *>(&attr[i]),
        (*paths)[i].mode, nullptr, nullptr);
    attr[i].attr.uid = (*paths)[i].uid;
    attr[i].attr.gid = (*paths)[i].gid;
    attr[i].attr.rdev = (*paths)[i].rdev;
    attr[i].attr.nbytes = (*paths)[i].nbytes;
    attr[i].driver = driver;
    attr[i].user_attr = (*paths)[i].attr;

    int resflags = 0;
    if (config.self) resflags |= _RESMGR_FLAG_SELF;
    switch ((*paths)[i].order) {
      case path_order::before:
        resflags |= _RESMGR_FLAG_BEFORE;
        break;
      case path_order::after:
        resflags |= _RESMGR_FLAG_AFTER;
        break;
      default:
        // Default, no flags to set.
        break;
    }

    ids[i] = resmgr_attach(dpp_,   // dispatch handle
        &resmgr_attr,              // resource mgr attributes
        (*paths)[i].path.c_str(),  // device name
        _FTYPE_ANY,                // open type
        resflags,                  // flags
        connect,                   // connection functions
        io,                        // I/O functions
        reinterpret_cast<iofunc_attr_t *>(
            &attr[i]));  // resource manager handle

    if (ids[i] == -1) {
      auto errno_cpy = errno;
      free(ids, i);
      wait_set();
      return stdext::unexpected{-errno_cpy};
    }
  }

  ctp_ = dispatch_context_alloc(dpp_);
  if (ctp_ == nullptr) {
    auto errno_cpy = errno;
    free(ids, path_count);
    wait_set();
    return stdext::unexpected{-errno_cpy};
  }

  // We can't set this based on:
  //
  //  running_ = exit_requested_ != DISPATCH_NO_EXIT`
  //
  // as that would require a read and a write, which is non-atomic. An error
  // would occur if `exit_requested_` is read as `DISPATCH_NO_EXIT`. Then this
  // thread is blocked, `exit()` is called, sets `exit_requested_` to
  // `DISPATCH_EXIT_REQUESTED`. It sees that `running_` is still false (it
  // hasn't been set yet) and won't send the pulse. But then `running_` is set
  // to `true` from the read operation and potentially blocks forver in
  // `dispatch_block()` as there will never be a `dispatch_unblock()` to
  // unblock it allowing an exit as expected.
  running_.store(true);
  wait_set();

  while (true) {
    if (dispatch_block(ctp_) == nullptr) {
      if (errno != EINTR) {
        // Can't safely exit in the loop, as this could lead to a race
        // condition with the exit() function resulting in undefined
        // behaviour.
        std::abort();
      }
    } else {
      dispatch_handler(ctp_);
    }
    if (exit_requested_.load()) break;
  }

  free(ids, path_count);
  return exit_code_;
}

auto device::exit(int code) noexcept -> void {
  // The exit code must be set before setting exit_requested_, else calling
  // run() may see that an exit is requested and return the exit_code_ before
  // it is set. That means, every call to exit() can modify the exit code.
  exit_code_ = code;

  // Request to exit. Ignore all requests after the first one.
  int expected = DISPATCH_NO_EXIT;
  if (!std::atomic_compare_exchange_strong(
          &exit_requested_, &expected, DISPATCH_EXIT_REQUESTED))
    return;

  // This line will only be reached ever once for the resource manager. If it
  // does get here, then `running_` must be true, which means that the `ctp_`
  // must be valid. The dispatch loop may never exit unless `exit_requested_`
  // has a value other than `DISPATCH_NO_EXIT` (0).
  if (running_.load()) dispatch_unblock(ctp_);
}

auto device::free(const std::vector<int> ids, std::size_t count) noexcept
    -> void {
  running_.store(false);

  // exit() will never call here with `dpp_` or `ctp_` uninitialised, because
  // after `running_` is `true`, only `exit()` can cause the loop, and
  // `free()` should never be called from witin the dispatch handler loop.
  if (dpp_) {
    if (ctp_) {
      dispatch_context_free(ctp_);
      ctp_ = nullptr;
    }

    for (std::size_t i = 0; i < count; i++) {
      resmgr_detach(dpp_, ids[i], _RESMGR_DETACH_ALL);
    }
    dispatch_destroy(dpp_);
    dpp_ = nullptr;
  }
}

}  // namespace details

}  // namespace os::qnx::resmgr
