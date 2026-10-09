#include "cpuid-driver.h"

#include "ubench/thread.h"

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

auto cpuid_driver::read(resmgr_context_t*, io_read_t* msg, RESMGR_OCB_T* ocb,
    [[maybe_unused]] attr_type* attr) noexcept -> int {
  return ENOSYS;
}

auto cpuid_driver::lseek(resmgr_context_t*, io_lseek_t* msg, RESMGR_OCB_T* ocb,
    [[maybe_unused]] attr_type* attr) noexcept -> int {
  return ENOSYS;
}
