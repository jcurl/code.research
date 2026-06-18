#include "payload.h"

#include <sstream>

#include "sjson/json_writer.h"

using namespace ubench::net;
using namespace ubench::sjson;

namespace std {

/// @brief Converts an interface to "addr/netmask".
///
/// @param addr to convert.
///
/// @return a string representation of the address.
auto to_string(const if_ipv4& addr) -> std::string {
  int netmask = -1;
  if (addr.netmask()) {
    // False positive.
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    in_addr_t n = ntohl(addr.netmask()->s_addr);

    // Count consecutive set bits from MSB, stopping at first zero bit
    int bitcount = 0;
    in_addr_t mask = 0x80000000;
    while ((n & mask) && mask != 0) {
      bitcount++;
      mask >>= 1;
    }
    netmask = bitcount;
  }

  auto ipv4 = inet_ntos(addr.addr());
  if (netmask != -1) {
    return ipv4 + "/" + std::to_string(netmask);
  } else {
    return ipv4;
  }
}

auto to_string(const ubench::net::ether_addr& addr) -> std::string {
  return ubench::net::ether_ntos(addr);
}

}  // namespace std

namespace {

// Writes the "ipv4" block in an object
auto create_net_iface(json_writer_object& block_iface, const interface& iface)
    -> void {
  if (iface.vlan().has_value()) {
    // False positive.
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    block_iface.write_value("vlan", iface.vlan()->id);
  }
  if (iface.hw_addr().has_value()) {
    // False positive.
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    block_iface.write_value("mac", *iface.hw_addr());
  }
  if (iface.mtu()) {
    // False positive.
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    block_iface.write_value("mtu", *iface.mtu());
  }
  auto block_ipv4 = block_iface.write_array("ipv4");
  for (const auto& ipv4 : iface.inet()) {
    block_ipv4.write_value(ipv4);
  }
}

// Writes the "interfaces" block in an object.
auto create_net_block(json_writer_object& block) -> void {
  const auto hostname = gethostname();
  if (hostname) {
    block.write_value("host", *hostname);
  }

  const auto domainname = getdomainname();
  if (domainname) {
    block.write_value("domain", *domainname);
  }

  auto block_ifaces = block.write_object("interfaces");
  auto interfaces = ubench::net::query_net_interfaces();
  for (const auto& [name, iface] : interfaces) {
    const auto active = if_flags::UP | if_flags::RUNNING;
    if ((iface.status() & active) == active) {
      auto block_iface = block_ifaces.write_object(name);
      create_net_iface(block_iface, iface);
    }
  }
}

}  // namespace

auto payload::generate() -> std::string {
  auto ss = std::stringstream{};
  {
    auto writer = ubench::sjson::json_writer(ss);
    writer.config().escape_solidus = false;
    {
      auto block = writer.write_object();
      create_net_block(block);
    }
  }
  return ss.str();
}
