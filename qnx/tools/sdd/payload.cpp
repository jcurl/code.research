#include "payload.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

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

/// @brief Converts an Ethernet interface to colon notation.
///
/// @param addr to convert.
///
/// @return a string representation of the address.
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

payload::payload(const sockaddr_in& dest) {
  if (!is_multicast(dest.sin_addr)) {
    throw std::invalid_argument("Expected multicast destination");
  }

  // We throw away the cast, as it's not good having const private members in a
  // class. It's a copy we make.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  dest_ = const_cast<sockaddr_in&>(dest);
}

auto payload::open(const sockaddr_in& bind, unsigned int mtu)
    -> stdext::expected<void, int> {
  // Ensure it is valid.
  if (bind.sin_addr.s_addr == INADDR_ANY ||
      bind.sin_addr.s_addr == INADDR_NONE) {
    return stdext::unexpected{EINVAL};
  }

  // Ensure it doesn't already exist.
  for (auto& s : sockets_) {
    if (s.src.sin_addr.s_addr == bind.sin_addr.s_addr &&
        s.src.sin_port == bind.sin_port) {
      s.active = true;
      return stdext::unexpected{EEXIST};
    }
  }

  iface_ctx in{};
  in.udp = udp{};

  auto oresult = in.udp.open(bind, dest_);
  if (!oresult) return stdext::unexpected{oresult.error()};

  auto tresult = in.udp.set_multicast_ttl(1);
  if (!tresult) return stdext::unexpected{tresult.error()};

  if (mtu) {
    in.mtu = mtu;
    auto hresult = in.udp.ipv4hdr_length();
    if (hresult) in.ipv4hdr = *hresult;
  }

  in.src = bind;
  in.active = true;
  sockets_.push_back(std::move(in));
  return {};
}

auto payload::close(const sockaddr_in& bind) -> stdext::expected<void, int> {
  for (auto it = sockets_.begin(); it != sockets_.end();) {
    auto& in = *it;
    if (in.src.sin_addr.s_addr == bind.sin_addr.s_addr &&
        in.src.sin_port == bind.sin_port) {
      // This is the socket. Close it.
      in.udp.close();
      sockets_.erase(it);
      return {};
    } else {
      it++;
    }
  }
  return stdext::unexpected{ESRCH};
}

auto payload::send() -> stdext::expected<void, int> {
  auto p = generate();

  int e = 0;
  for (auto& ctx : sockets_) {
    if (ctx.mtu == 0 || (p.length() < ctx.mtu - ctx.ipv4hdr - 8)) {
      if (!ctx.udp.send(p)) {
        if (!e) e = errno;
      }
    }
  }
  if (e) return stdext::unexpected{e};
  return {};
}

namespace {

constexpr auto IFACE_READY =
    if_flags::MULTICAST | if_flags::UP | if_flags::RUNNING;
constexpr auto IFACE_MASK = IFACE_READY | if_flags::LOOPBACK;

auto is_up(ubench::flags<if_flags> flags) -> bool {
  return (flags & IFACE_MASK) == IFACE_READY;
}

}  // namespace

auto payload::set_inactive() -> void {
  for (auto& ctx : sockets_) {
    ctx.active = false;
  }
}

auto payload::close_inactive() -> void {
  for (auto it = sockets_.begin(); it != sockets_.end();) {
    auto& in = *it;
    if (!in.active) {
      std::cout << "Closed: " << ubench::net::inet_ntos(in.src) << std::endl;
      in.udp.close();
      sockets_.erase(it);
    } else {
      it++;
    }
  }
}

auto payload::query(in_port_t srcport) -> bool {
  set_inactive();

  auto interfaces = query_net_interfaces();
  for (const auto& [name, intf] : interfaces) {
    if (is_up(intf.status())) {
      for (const auto& ipv4 : intf.inet()) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr = ipv4.addr();
        addr.sin_port = srcport;
        unsigned int mtu{};
        if (intf.mtu()) {
          // False positive clang-tidy 18.1.3
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          mtu = *intf.mtu();
        }
        auto result = open(addr, mtu);
        if (result) {
          std::cout << "Opened: " << ubench::net::inet_ntos(addr) << std::endl;
        }
      }
    }
  }

  // Look for all sockets that don't have an active interface and remove them.
  close_inactive();
  return !sockets_.empty();
}

auto payload::query(std::string iface, in_port_t srcport) -> bool {
  set_inactive();

  auto pintf = query_net_interface(std::move(iface));
  if (pintf) {
    auto& intf = *pintf;
    if (is_up(intf.status())) {
      for (const auto& ipv4 : intf.inet()) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr = ipv4.addr();
        addr.sin_port = srcport;
        unsigned int mtu{};
        if (intf.mtu()) {
          // False positive clang-tidy 18.1.3
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          mtu = *intf.mtu();
        }
        auto result = open(addr, mtu);
        if (result) {
          std::cout << "Opened: " << ubench::net::inet_ntos(addr) << std::endl;
        }
      }
    }
  }

  // Look for all sockets that don't have an active interface and remove them.
  close_inactive();
  return !sockets_.empty();
}

auto payload::query(const sockaddr_in& iface) -> bool {
  set_inactive();

  auto interfaces = query_net_interfaces();
  for (const auto& [name, intf] : interfaces) {
    if (is_up(intf.status())) {
      for (const auto& ipv4 : intf.inet()) {
        if (iface.sin_addr.s_addr == ipv4.addr().s_addr) {
          unsigned int mtu{};
          if (intf.mtu()) {
            // False positive clang-tidy 18.1.3
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            mtu = *intf.mtu();
          }
          auto result = open(iface, mtu);
          if (result) {
            std::cout << "Opened: " << ubench::net::inet_ntos(iface)
                      << std::endl;
          }
          goto found;
        }
      }
    }
  }

found:
  // Look for all sockets that don't have an active interface and remove them.
  close_inactive();
  return !sockets_.empty();
}