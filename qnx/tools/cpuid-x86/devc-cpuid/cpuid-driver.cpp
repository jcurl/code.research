#include "cpuid-driver.h"

#include <vector>

#include "ubench/thread.h"

namespace {

auto get_cpuid(rjcp::cpuid::cpuidreader_cache& cpuid, unsigned int core,
    std::uint32_t eax, std::uint32_t ecx, std::size_t count)
    -> stdext::expected<std::vector<rjcp::cpuid::cpuid_res>, int> {
  if (!cpuid.has_cpuid()) return stdext::unexpected{ENOSYS};
  if (count == 0) return {};

  std::vector<rjcp::cpuid::cpuid_res> res{};
  res.reserve(count);

  auto ctx = cpuid.enable_core(core);
  if (!ctx) return stdext::unexpected{EIO};

  for (std::size_t i = 0; i < count; i++) {
    auto regs = cpuid.cpuid(eax + i, ecx);
    if (!regs) return stdext::unexpected{EIO};
    res.emplace_back(std::move(*regs));
  }
  return res;
}

}  // namespace

auto cpuid_driver::init_cpuid_driver() -> void {
  static const std::string basedev{"/dev/cpu/"};

  const auto threads = ubench::thread::thread_count();
  if (threads > 0) {
    std::vector<attr_type> attrs{};
    attrs.reserve(threads);

    std::vector<os::qnx::resmgr::rsrc<attr_type>> devices{};
    devices.reserve(threads);
    for (std::size_t i = 0; i < threads; i++) {
      // Reserving space means that no reallocation will occur when emplacing
      // data at the end, so we can safely take the pointer later. Emplacing
      // more than what we reserved could result in a change in allocation and
      // then bad pointers to devices.
      attrs.emplace_back(attr_type{static_cast<unsigned int>(i)});
      devices.emplace_back(os::qnx::resmgr::rsrc<attr_type>{
          .path = basedev + std::to_string(i) + "/cpuid",
          .order = os::qnx::resmgr::path_order::default_order,
          .mode = S_IFNAM | 0666,
          .uid = 0,
          .gid = 0,
          .rdev = makedev(0, 203, i),
          .nbytes = 0,
          .attr = &attrs[i]});
    }
    attrs_ = std::move(attrs);
    devices_ = std::move(devices);
  }
}

auto cpuid_driver::read(resmgr_context_t* ctp, io_read_t* msg,
    RESMGR_OCB_T* ocb, attr_type* attr) noexcept -> int {
  // We don't care if the user specified O_NONBLOCK or not.
  auto verify = iofunc_read_verify(ctp, msg, ocb, nullptr);
  if (verify != EOK) return _RESMGR_ERRNO(verify);

  auto xtype = msg->i.xtype & _IO_XTYPE_MASK;
  if (xtype & _IO_XTYPE_OFFSET) {
    // pread / pread64.
    auto pread_msg = as_pread_t(msg);
    std::uint32_t eax = pread_msg->offset.offset & 0xFFFFFFFF;
    std::uint32_t ecx = (pread_msg->offset.offset >> 32) & 0xFFFFFFFF;

    int numbytes = _IO_READ_GET_NBYTES(msg);
    if (numbytes % 16 != 0) return _RESMGR_ERRNO(EINVAL);

    auto regs = get_cpuid(cpuid_cache_, attr->core, eax, ecx, numbytes >> 4);
    if (!regs) return _RESMGR_ERRNO(regs.error());
    if (regs->size() == 0) return _RESMGR_NPARTS(0);

    MsgReply(ctp->rcvid, numbytes, regs->data(), numbytes);
    return _RESMGR_NOREPLY;
  } else if (xtype == _IO_XTYPE_NONE) {
    // read / read64.
    std::uint32_t eax = ocb->offset & 0xFFFFFFFF;
    std::uint32_t ecx = (ocb->offset >> 32) & 0xFFFFFFFF;

    int numbytes = _IO_READ_GET_NBYTES(msg);
    if (numbytes % 16 != 0) return _RESMGR_ERRNO(EINVAL);

    auto regs = get_cpuid(cpuid_cache_, attr->core, eax, ecx, numbytes >> 4);
    if (!regs) return regs.error();
    if (regs->size() == 0) return _RESMGR_NPARTS(0);
    ocb->offset += numbytes >> 4;

    MsgReply(ctp->rcvid, numbytes, regs->data(), numbytes);
    return _RESMGR_NOREPLY;
  } else {
    // We don't support this extended message.
    return _RESMGR_ERRNO(ENOSYS);
  }
}
