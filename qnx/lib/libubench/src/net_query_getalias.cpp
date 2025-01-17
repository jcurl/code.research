#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <cstring>
#include <string>

#include "ubench/net.h"
#include "ubench/file.h"

#include "net_common.h"
#include "stringext.h"

namespace ubench::net {

auto query_net_interface_friendly_name(const ubench::file::fdesc& sock, const std::string& interface) -> std::optional<std::string> {
  // This implementation is for Cygwin.
  if (!sock) return {};

  ifreq ifr;
  ifreq_frndlyname ifn;
  ifr.ifr_frndlyname = &ifn;
  strlcpy(&ifr.ifr_name[0], interface.c_str(), IFNAMSIZ);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (ioctl(sock, SIOCGIFFRNDLYNAM, &ifr) < 0) return {};

  return std::string{ifn.ifrf_friendlyname};
}

}
