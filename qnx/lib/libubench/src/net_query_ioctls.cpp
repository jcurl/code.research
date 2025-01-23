#include "config.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <unistd.h>

#include <cstring>
#include <string>

#include "ubench/file.h"
#include "ubench/net.h"
#include "ubench/string.h"
#include "net_common.h"

namespace ubench::net {

auto query_net_interface_flags(const ubench::file::fdesc& sock,
    const std::string& interface) -> std::optional<unsigned int> {
  if (!sock) return {};

  ifreq ifr{};
  strlcpy(&ifr.ifr_name[0], interface.c_str(), IFNAMSIZ);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) return {};

  // On Linux, the `ifr_flags` is an `int`. On FreeBSD, it's a `short &` (as it
  // is a reference to the first element in an array of `short`). We want to put
  // the flags from a potentially smaller datatype into a larger, now unsigned,
  // but do not want the sign extension either usually associated with such type
  // casts.
  using uflag_t =
      std::make_unsigned_t<std::remove_reference_t<decltype(ifr.ifr_flags)>>;
  unsigned int flags = static_cast<uflag_t>(ifr.ifr_flags);

#if HAVE_NET_IFR_FLAGS_HIGH
  // We need to merge the flags high to the flags. Assume this is the same size
  // as the flags field. Ensure of we don't get undefined behaviour by shifting
  // more than the destination operand size.
  if (sizeof(unsigned int) > sizeof(uflag_t)) {
    flags |= static_cast<uflag_t>(ifr.ifr_flagshigh) << (sizeof(uflag_t) * 8);
  }
#endif

  return flags;
}

auto query_net_interface_mtu(const ubench::file::fdesc& sock,
    const std::string& interface) -> std::optional<unsigned int> {
  if (!sock) return {};

  ifreq ifr{};
  strlcpy(&ifr.ifr_name[0], interface.c_str(), IFNAMSIZ);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (ioctl(sock, SIOCGIFMTU, &ifr) < 0) return {};

  return ifr.ifr_mtu;
}

// ----------------------------------------------------------------------------
// Stubs. These are required when the OS doesn't provide the necessary
// functionality. If the OS provides such functionality, then they're define in
// an OS specific unit.cpp file.
//
// The signatures are defined in "net_common.h". Having this here declutters
// other code by not needing to always use #define checks. They are required,
// else linkage will fail. We can't do this in "net_common.h" else the symbols
// will be defined multiple times and result in a linkage failure.

#if !HAVE_NET_GETIFADDRS
auto query_net_interfaces_getifaddrs(
    [[maybe_unused]] std::map<std::string, interface>& interfaces) -> void {}
#endif

#if !HAVE_NET_SIOCGIFFRNDLYNAM
auto query_net_interface_friendly_name(
    [[maybe_unused]] const ubench::file::fdesc& sock,
    [[maybe_unused]] const std::string& interface)
    -> std::optional<std::string> {
  return {};
}
#endif

#if !HAVE_NET_SIOCGIFHWADDR
auto query_net_interface_hw_addr(
    [[maybe_unused]] const ubench::file::fdesc& sock,
    [[maybe_unused]] const std::string& interface)
    -> std::optional<ether_addr> {
  return {};
}
#endif

#if !HAVE_NET_VLAN
auto query_net_interface_vlan_id(
    [[maybe_unused]] const ubench::file::fdesc& sock,
    [[maybe_unused]] const std::string& interface) -> std::optional<if_vlan> {
  return {};
}
#endif

}  // namespace ubench::net
