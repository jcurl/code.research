#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>

#include "stdext/expected.h"
#include "ubench/file.h"
#include "ubench/string.h"
#include "net_common.h"

namespace ubench::net {

auto query_net_interface_friendly_name(const ubench::file::fdesc& sock,
    const std::string& interface) -> stdext::expected<std::string, int> {
  // This implementation is for Cygwin.
  if (!sock) return stdext::unexpected{EBADF};

  ifreq ifr;
  ifreq_frndlyname ifn;
  ifr.ifr_frndlyname = &ifn;
  strlcpy(&ifr.ifr_name[0], interface.c_str(), IFNAMSIZ);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (ioctl(sock, SIOCGIFFRNDLYNAM, &ifr) < 0) return stdext::unexpected{errno};

  return std::string{ifn.ifrf_friendlyname};
}

}  // namespace ubench::net
