#include "config.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#if HAVE_NET_INCLUDE_NET_IF_VLANVAR_H
#include "net/if_ether.h"
#include "net/if_vlanvar.h"
#endif

#if HAVE_NET_INCLUDE_NET_IF_VLAN_VAR_H
#include "net/if.h"
#include "net/if_vlan_var.h"
#endif

#include <cerrno>
#include <cstring>

#include "stdext/expected.h"
#include "ubench/string.h"
#include "net_common.h"

namespace ubench::net {

auto query_net_interface_vlan_id(const ubench::file::fdesc& sock,
    const std::string& interface) -> stdext::expected<if_vlan, int> {
  // This implementation is for QNX 7.1 and NetBSD.
  if (!sock) return stdext::unexpected{EBADF};

  ifreq ifr{};
  vlanreq req{};
#if HAVE_NET_INCLUDE_NET_IF_VLANVAR_H
  // NetBSD, QNX 7.1
  ifr.ifr_data = &req;
#else
  // FreeBSD, QNX 8.0
  ifr.ifr_data = (caddr_t)&req;
#endif

  strlcpy(&ifr.ifr_name[0], interface.c_str(), IFNAMSIZ);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (ioctl(sock, SIOCGETVLAN, &ifr) < 0) return stdext::unexpected{errno};

  if_vlan result{};
  result.parent = std::string{req.vlr_parent};
  result.id = req.vlr_tag;
  return result;
}

}  // namespace ubench::net
