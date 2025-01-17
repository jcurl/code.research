#include "config.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#if HAVE_NET_INCLUDE_LINUX_SOCKIOS_H
#include <linux/if_vlan.h>
#include <linux/sockios.h>
#endif

#include <cerrno>
#include <cstring>

#include "net_common.h"
#include "stringext.h"

namespace ubench::net {

auto query_net_interface_vlan_id(const ubench::file::fdesc& sock,
    const std::string& interface) -> std::optional<if_vlan> {
  // This implementation is for Linux.
  if (!sock) return {};

  if_vlan result{};
  vlan_ioctl_args ifv{};

  ifv.cmd = GET_VLAN_REALDEV_NAME_CMD;
  strlcpy(&ifv.device1[0], interface.c_str(), sizeof(ifv.device1));

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (ioctl(sock, SIOCGIFVLAN, &ifv) < 0) {
    // Not a VLAN device.
    return {};
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access,cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  result.parent = std::string{ifv.u.device2};

  ifv.cmd = GET_VLAN_VID_CMD;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (ioctl(sock, SIOCGIFVLAN, &ifv) < 0) return {};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
  result.id = ifv.u.VID;

  return result;
}

}  // namespace ubench::net
