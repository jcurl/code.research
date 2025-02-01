#include "config.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <unistd.h>
#if HAVE_NET_INCLUDE_NET_ETHERNET_H
// Present on Linux, provides ::ether_addr
#include <net/ethernet.h>
#include <net/if_arp.h>
#endif

#include <cerrno>
#include <cstring>

#include "ubench/file.h"
#include "ubench/net.h"
#include "ubench/string.h"
#include "net_common.h"

namespace ubench::net {

auto query_net_interface_hw_addr(const ubench::file::fdesc& sock,
    const std::string& interface) -> stdext::expected<ether_addr, int> {
  // This implementation is for Cygwin and Linux.
  if (!sock) return stdext::unexpected{EBADF};

  ifreq ifr{};
  ifr.ifr_addr.sa_family = AF_INET;
  strlcpy(&ifr.ifr_name[0], interface.c_str(), IFNAMSIZ);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) return stdext::unexpected{errno};

#if HAVE_NET_INCLUDE_NET_ETHERNET_H
  if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
    return stdext::unexpected(ENOTSUP);

  // Linux provides ether_addr in the global namespace.
  std::uint8_t* native_hw_addr =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      ((::ether_addr*)&ifr.ifr_hwaddr.sa_data)->ether_addr_octet;
  constexpr size_t ETHLEN = ETH_ALEN;
#else
  // Cygwin 1.7.x and later doesn't have ether_addr. Use `sockaddr`
  // direct.
  std::uint8_t* native_hw_addr =
      reinterpret_cast<std::uint8_t*>(ifr.ifr_hwaddr.sa_data);
  constexpr size_t ETHLEN = ether_addr::ETH_ADDR_LEN;
#endif

  // We might get all zeroes, which should be interpreted as having no HW
  // address.

  bool is_zero = true;
  for (size_t i = 0; i < ETHLEN; i++) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (native_hw_addr[i] != 0) {
      is_zero = false;
      break;
    }
  }
  if (is_zero) return stdext::unexpected(ENOTSUP);

  ether_addr hwaddr{};
  std::memcpy(&hwaddr.ether_addr_octet, native_hw_addr, ETHLEN);
  return hwaddr;
}

}  // namespace ubench::net
