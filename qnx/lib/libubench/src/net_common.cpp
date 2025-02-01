#include "config.h"

#include "net_common.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <cstdint>

#include "ubench/net.h"

namespace ubench::net {

auto is_multicast(const in_addr& addr) -> bool {
  return (ntohl(addr.s_addr) & 0xF0000000) == 0xE0000000;
}

auto get_ether_multicast(const in_addr& addr, ether_addr* eth) -> bool {
  if (eth == nullptr) return false;
  if (!is_multicast(addr)) return false;

  std::uint32_t ipv4addr = ntohl(addr.s_addr);
  eth->ether_addr_octet[0] = 0x01;
  eth->ether_addr_octet[1] = 0x00;
  eth->ether_addr_octet[2] = 0x5E;
  eth->ether_addr_octet[3] = (ipv4addr >> 16) & 0x7F;
  eth->ether_addr_octet[4] = (ipv4addr >> 8) & 0xFF;
  eth->ether_addr_octet[5] = ipv4addr & 0xFF;
  return true;
}

auto inet_ntos(const sockaddr_in& addr) -> std::string {
  // Wrong type of address (probably typecasting issue).
  if (addr.sin_family != AF_INET) return {};

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  char paddr[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &(addr.sin_addr), (char*)paddr, sizeof(paddr)) ==
      nullptr)
    return {};
  std::string result{};
  result.resize(22);

  // The `snprintf` is about 2x faster than creating a stringstream, writing to
  // it, and then extracting the string from it.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  int len = std::snprintf(
      result.data(), 22, "%s:%d", (char*)paddr, ntohs(addr.sin_port));
  result.resize(len);
  return result;
}

auto inet_ntos(const in_addr& addr) -> std::string {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  char paddr[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &addr, (char*)paddr, sizeof(paddr)) == nullptr)
    return {};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  return std::string{paddr};
}

auto ether_ntos(const ether_addr& addr) -> std::string {
  std::string result{};
  result.resize(18);

  // The `snprintf` is about 2x faster than creating a stringstream, writing to
  // it, and then extracting the string from it.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  int len = std::snprintf(result.data(), 18, "%02x:%02x:%02x:%02x:%02x:%02x",
      addr.ether_addr_octet[0], addr.ether_addr_octet[1],
      addr.ether_addr_octet[2], addr.ether_addr_octet[3],
      addr.ether_addr_octet[4], addr.ether_addr_octet[5]);
  result.resize(len);
  return result;
}

auto if_ipv4::operator==(const if_ipv4& rhs) const noexcept -> bool {
  if (addr_.s_addr != rhs.addr_.s_addr) return false;
  if ((baddr_ && !rhs.baddr_) || (!baddr_ && rhs.baddr_)) return false;
  if ((daddr_ && !rhs.daddr_) || (!daddr_ && rhs.daddr_)) return false;
  if ((nmask_ && !rhs.nmask_) || (!nmask_ && rhs.nmask_)) return false;
  if (baddr_ && baddr_->s_addr != rhs.baddr_->s_addr) return false;
  if (daddr_ && daddr_->s_addr != rhs.daddr_->s_addr) return false;
  if (nmask_ && nmask_->s_addr != rhs.nmask_->s_addr) return false;
  return true;
}

namespace {

auto query_net_interfaces_filter(std::optional<std::string> query)
    -> const std::map<std::string, interface> {
  // Query for all or some interfaces. We can return all interfaces, or only
  // return a single interface. Even for a single interface we must query all
  // interfaces that we get all IP aliases (not possible with SIOCGIFADDR which
  // will only return the preferred address for the interface).

  std::map<std::string, interface> interfaces{};

  if (HAVE_NET_GETIFADDRS) {
    query_net_interfaces_getifaddrs(interfaces);
  } else {
    // We don't have any other way to query all interfaces (e.g. SIOCGIFCONF)
    return interfaces;
  }

  // Query individual elements via ioctl()
  ubench::file::fdesc sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock) {
    for (auto& interface : interfaces) {
      // If no query, then get information for all. If a query, then only fill
      // in missing details for the interface in question.
      if (!query || *query == interface.first) {
        if (HAVE_NET_SIOCGIFFRNDLYNAM) {
          auto alias = query_net_interface_friendly_name(sock, interface.first);
          if (alias) interface.second.alias(*alias);
        }

        if (HAVE_NET_SIOCGIFHWADDR) {
          if (!interface.second.hw_addr()) {
            auto hwaddr = query_net_interface_hw_addr(sock, interface.first);
            if (hwaddr) interface.second.hw_addr(*hwaddr);
          }
        }

        if (HAVE_NET_VLAN) {
          auto vlan = query_net_interface_vlan_id(sock, interface.first);
          if (vlan) {
            // We can ignore the return value, as it will then be unset, which
            // is required behaviour (the VLAN ID is outside the specified
            // range).
            (void)interface.second.vlan(*vlan);

            // Fix the HW address. On NetBSD, QNX 7.1, the VLAN interface gets
            // the HW address via AF_LINK. On FreeBSD, QNX 8.0 it's missing so
            // copy it from the parent interface. We know we have the HW address
            // already because it was obtained by an AF_LINK query in
            // getifaddrs().
            if (!HAVE_NET_SIOCGIFHWADDR) {
              // Don't need to query, if we did already.
              if (!interface.second.hw_addr()) {
                auto parent = interfaces.find(vlan->parent);
                if (parent != interfaces.end() && parent->second.hw_addr()) {
                  interface.second.hw_addr(parent->second.hw_addr());
                }
              }
            }
          }
        }

        auto mtu = query_net_interface_mtu(sock, interface.first);
        if (mtu) interface.second.mtu(*mtu);

#if __CYGWIN__
        // For Cygwin, we get the flags explicitly. The flags that comes in with
        // getifaddrs() is correct, but returns a different value than the ioctl
        // query SIOCGIFFLAGS. e.g. for IPv4 it might be 0x11043, but for IPv6
        // returns 0x11041 (IFF_BROADCAST). While technically correct, it would
        // be confusing.
        //
        // On FreeBSD and QNX8 we must use SIOCGIFFLAGS to get the high flags
        // value, which isn't present in the getiffaddrs() implementation.
        auto flags = query_net_interface_flags(sock, interface.first);
        if (flags) interface.second.flags(*flags);
#endif
      }
    }
  }
  return interfaces;
}

}  // namespace

auto query_net_interfaces() -> const std::map<std::string, interface> {
  return query_net_interfaces_filter(std::nullopt);
}

auto query_net_interface(std::string name) -> const std::optional<interface> {
  auto interfaces = query_net_interfaces_filter(name);
  auto intf = interfaces.find(name);
  if (intf == interfaces.end()) return {};

  // GCC 9.4.0 doesn't allow for an implicit return of `intf->second`.
  return std::optional{std::move(intf->second)};
}

auto interface::status() const noexcept -> const ubench::flags<if_flags> {
  ubench::flags<if_flags> f{};
  if (flags_ & IFF_UP) f |= if_flags::UP;
  if (flags_ & IFF_BROADCAST) f |= if_flags::BROADCAST;
  if (flags_ & IFF_LOOPBACK) f |= if_flags::LOOPBACK;
  if (flags_ & IFF_POINTOPOINT) f |= if_flags::POINTOPOINT;
  if (flags_ & IFF_RUNNING) f |= if_flags::RUNNING;
  if (flags_ & IFF_NOARP) f |= if_flags::NOARP;
  if (flags_ & IFF_MULTICAST) f |= if_flags::MULTICAST;
#if HAVE_NET_IFF_LOWER_UP
  if (flags_ & IFF_LOWER_UP) f |= if_flags::LOWER_UP;
#endif
  return f;
}

}  // namespace ubench::net

auto operator<<(std::ostream& os, const sockaddr_in& addr) -> std::ostream& {
  return os << ubench::net::inet_ntos(addr);
}

auto operator<<(std::ostream& os, const in_addr& addr) -> std::ostream& {
  return os << ubench::net::inet_ntos(addr);
}

auto operator<<(std::ostream& os, const ubench::net::ether_addr& addr)
    -> std::ostream& {
  return os << ubench::net::ether_ntos(addr);
}
