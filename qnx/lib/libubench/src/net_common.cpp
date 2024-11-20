#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <cstdint>
#include <functional>
#include <iomanip>
#include <sstream>

// DEBUG
#include <iostream>

#include "ubench/net.h"

#include "config.h"

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

auto inet_ntos(const sockaddr_in& addr) -> const std::string {
  // Wrong type of address (probably typecasting issue).
  if (addr.sin_family != AF_INET) return {};

  // C/C++ interop with the OS.
  //
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

auto inet_ntos(const in_addr& addr) -> const std::string {
  // C/C++ interop with the OS.
  //
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  char paddr[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &addr, (char*)paddr, sizeof(paddr)) == nullptr)
    return {};

  // C/C++ interop with the OS.
  //
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  return std::string{paddr};
}

auto ether_ntos(const ether_addr& addr) -> const std::string {
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

// Forward declarations. Don't need this in a header file.
auto query_net_interfaces_getifaddrs(std::function<bool(
        std::string, unsigned int, sockaddr*, sockaddr*, sockaddr*, sockaddr*)>
        cb) -> void;

auto get_ipv4(sockaddr* addr, sockaddr* netmask, sockaddr* brdaddr,
    sockaddr* dstaddr) -> std::optional<if_ipv4> {
  if (!addr) return {};
  if (addr->sa_family != AF_INET) return {};

  in_addr* ipv4_netmask = nullptr;
  in_addr* ipv4_brdaddr = nullptr;
  in_addr* ipv4_dstaddr = nullptr;

  if (netmask && netmask->sa_family == AF_INET)
    ipv4_netmask = &((sockaddr_in*)(netmask))->sin_addr;

  if (brdaddr && brdaddr->sa_family == AF_INET)
    ipv4_brdaddr = &((sockaddr_in*)(brdaddr))->sin_addr;

  if (dstaddr && dstaddr->sa_family == AF_INET)
    ipv4_dstaddr = &((sockaddr_in*)(dstaddr))->sin_addr;

  return if_ipv4{
      ((sockaddr_in*)addr)->sin_addr, ipv4_netmask, ipv4_brdaddr, ipv4_dstaddr};
}

auto query_net_interfaces() -> std::map<std::string, net_if> {
  std::map<std::string, net_if> interfaces{};

  if (HAVE_NET_GETIFADDRS) {
    query_net_interfaces_getifaddrs(
        [&interfaces](std::string name, unsigned int flags, sockaddr* addr,
            sockaddr* netmask, sockaddr* brdaddr, sockaddr* dstaddr) -> bool {
          auto [it, ins] = interfaces.try_emplace(name, net_if{});
          net_if* netif = &it->second;
          if (ins) netif->name_ = name;

          auto ipv4 = get_ipv4(addr, netmask, brdaddr, dstaddr);
          if (ipv4) netif->inet_.push_back(*ipv4);
          return true;
        });
  }

  // Debugging only.
  for (auto const& pair : interfaces) {
    std::cout << "Final Map: " << pair.first << "; " << pair.second.name()
              << std::endl;
    for (auto const& ip : pair.second.inet()) {
      std::cout << "  " << ip.addr() << std::endl;
    }
  }

  return interfaces;
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
