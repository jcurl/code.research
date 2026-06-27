#include "config.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#if HAVE_NET_INCLUDE_LINUX_SOCKIOS_H
#include <linux/if_vlan.h>
#include <linux/sockios.h>
#endif

#include <cerrno>
#include <cstring>

#include "stdext/expected.h"
#include "ubench/string.h"
#include "net_common.h"

namespace ubench::net {

auto query_net_interface_vlan_id(const ubench::file::fdesc& sock,
    const std::string& interface) -> stdext::expected<if_vlan, int> {
  // This implementation is for Linux.
  if (!sock) return stdext::unexpected{EBADF};

  if_vlan result{};
  vlan_ioctl_args ifv{};

  ifv.cmd = GET_VLAN_REALDEV_NAME_CMD;
  strlcpy(&ifv.device1[0], interface.c_str(), sizeof(ifv.device1));

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (ioctl(sock, SIOCGIFVLAN, &ifv) < 0) {
    // Not a VLAN device.
    return stdext::unexpected{errno};
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access,cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  result.parent = std::string{ifv.u.device2};

  ifv.cmd = GET_VLAN_VID_CMD;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (ioctl(sock, SIOCGIFVLAN, &ifv) < 0) return stdext::unexpected{errno};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
  result.id = ifv.u.VID;

  return result;
}

}  // namespace ubench::net
