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

auto to_string(const ether_addr& addr) -> std::string {
  return ether_ntos(addr);
}

}  // namespace std

namespace {

// Writes the "ipv4" block in a JSON object
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

// Writes the "interfaces" block in a JSON object.
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
  auto interfaces = query_net_interfaces();
  for (const auto& [name, iface] : interfaces) {
    const auto active = if_flags::UP | if_flags::RUNNING;
    if ((iface.status() & active) == active) {
      auto block_iface = block_ifaces.write_object(name);
      create_net_iface(block_iface, iface);
    }
  }
}

}  // namespace

// Generate a JSON object with network details.
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

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto payload::open(const sockaddr_in& bind, sockaddr_in dest)
    -> stdext::expected<int, int> {
  dest_ = dest;
  if (!is_multicast(dest.sin_addr)) {
    return stdext::unexpected{EINVAL};
  }

  int e = 0;
  auto interfaces = query_net_interfaces();
  for (const auto& [name, iface] : interfaces) {
    const auto ready = if_flags::MULTICAST | if_flags::UP | if_flags::RUNNING;
    const auto mask = ready | if_flags::LOOPBACK;

    if ((iface.status() & mask) == ready) {
      // Only use interfaces that are multicast and not loopback.
      for (const auto& ipv4 : iface.inet()) {
        if (bind.sin_addr.s_addr == INADDR_ANY ||
            ipv4.addr().s_addr == bind.sin_addr.s_addr) {
          iface_ctx in{};

          if (iface.mtu()) {
            // False positive.
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            in.mtu_ = *iface.mtu();
          }

          in.udp_ = udp{};
          sockaddr_in addr{};
          addr.sin_family = bind.sin_family;
          addr.sin_addr = ipv4.addr();
          addr.sin_port = bind.sin_port;

          if (in.udp_.open(addr)) {
            bool success = true;

            auto jresult = in.udp_.multicast_join(addr);
            if (!jresult) {
              success = false;
              e = jresult.error();
            }

            if (success) {
              auto tresult = in.udp_.set_multicast_ttl(1);
              if (!tresult) {
                success = false;
                e = tresult.error();
              }
            }

            if (success) {
              auto hresult = in.udp_.ipv4hdr_length();
              if (hresult) in.ipv4hdr_ = *hresult;
            }

            if (success) {
              sockets_.push_back(std::move(in));
            } else {
              in.udp_.close();
            }
          }
        }
      }
    }
  }

  if (sockets_.empty()) return stdext::unexpected{e};
  return sockets_.size();
}

auto payload::send() -> stdext::expected<void, int> {
  auto p = generate();

  int e = 0;
  for (auto& ctx : sockets_) {
    if (p.length() < ctx.mtu_ - ctx.ipv4hdr_) {
      if (!ctx.udp_.send(dest_, p)) {
        if (!e) e = errno;
      }
    }
  }
  if (e) return stdext::unexpected{e};
  return {};
}
