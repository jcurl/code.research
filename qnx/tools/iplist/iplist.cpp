#include <algorithm>
#include <iomanip>
#include <iostream>
#include <ostream>

#include "ubench/net.h"

namespace {

constexpr int wid_type = 8;
constexpr int wid_mtu = 7;
constexpr int wid_vlan = 4;
constexpr int wid_ipv4 = 15;
constexpr int wid_mac = 17;

class if_details final {
 public:
  if_details(int ifwidth, const ubench::net::interface& intf)
      : ifwidth_{ifwidth}, intf_{intf} {}
  if_details(const if_details&) = delete;
  auto operator=(const if_details&) -> if_details& = delete;
  if_details(if_details&&) = delete;
  auto operator=(if_details&&) -> if_details& = delete;
  ~if_details() = default;

  auto operator()(std::ostream& os) const -> std::ostream& {
    // Clang 19.1.4 FALSE POSITIVE.
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    std::string ifname = intf_.alias() ? *intf_.alias() : intf_.name();

    os << std::setw(ifwidth_) << ifname << " ";
    os << std::setw(wid_mac)
       // Clang 19.1.4 FALSE POSITIVE.
       // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
       << (intf_.hw_addr() ? ether_ntos(*intf_.hw_addr()) : "") << " ";
    os << std::setw(wid_mtu)
       // Clang 19.1.4 FALSE POSITIVE.
       // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
       << (intf_.mtu() ? std::to_string(*intf_.mtu()) : "") << " ";
    os << std::setw(wid_vlan)
       // Clang 19.1.4 FALSE POSITIVE.
       // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
       << (intf_.vlan() ? std::to_string(intf_.vlan()->id) : "") << " ";
    // Clang 19.1.4 FALSE POSITIVE.
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    os << std::setw(ifwidth_) << (intf_.vlan() ? intf_.vlan()->parent : "")
       << " ";
    return os;
  }

 private:
  int ifwidth_;

  // One of the few cases where a const reference is appropriate. The lifetime
  // of this class is expected to be very short, not copyable, not movable.
  const ubench::net::interface& intf_;
};

auto operator<<(std::ostream& out, const if_details& intf) -> std::ostream& {
  return intf(out);
}

class ipv4_details final {
 public:
  ipv4_details() = default;
  ipv4_details(const ubench::net::if_ipv4& ipv4) : ipv4_{&ipv4} {}
  ipv4_details(const ipv4_details&) = delete;
  auto operator=(const ipv4_details&) -> ipv4_details& = delete;
  ipv4_details(ipv4_details&&) = delete;
  auto operator=(ipv4_details&&) -> ipv4_details& = delete;
  ~ipv4_details() = default;

  auto operator()(std::ostream& os) const -> std::ostream& {
    if (ipv4_) {
      os << std::setw(wid_type) << "AF_INET"
         << " ";
      os << std::setw(wid_ipv4) << ipv4_->addr() << " ";
      os << std::setw(wid_ipv4)
         // Clang 19.1.4 FALSE POSITIVE.
         // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
         << (ipv4_->netmask() ? ubench::net::inet_ntos(*ipv4_->netmask()) : "")
         << " ";
      os << std::setw(wid_ipv4)
         << (ipv4_->broadcast_addr()
                    // Clang 19.1.4 FALSE POSITIVE.
                    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                    ? ubench::net::inet_ntos(*ipv4_->broadcast_addr())
                    : "")
         << " ";
      os << std::setw(wid_ipv4)
         // Clang 19.1.4 FALSE POSITIVE.
         // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
         << (ipv4_->dest_addr() ? ubench::net::inet_ntos(*ipv4_->dest_addr())
                                : "")
         << " ";
    } else {
      os << std::setw(wid_type) << ""
         << " ";
      os << std::setw(wid_ipv4) << ""
         << " ";
      os << std::setw(wid_ipv4) << ""
         << " ";
      os << std::setw(wid_ipv4) << ""
         << " ";
      os << std::setw(wid_ipv4) << ""
         << " ";
    }
    return os;
  }

 private:
  const ubench::net::if_ipv4* ipv4_{};
};

auto operator<<(std::ostream& out, const ipv4_details& ipv4) -> std::ostream& {
  return ipv4(out);
}

auto status(const ubench::net::interface& intf) -> std::string {
  std::string result{};
  result += (intf.status() & ubench::net::if_flags::UP) ? "U" : "-";
  result += (intf.status() & ubench::net::if_flags::LOWER_UP ? "u" : "-");
  result += (intf.status() & ubench::net::if_flags::RUNNING ? "R" : "-");
  result += (intf.status() & ubench::net::if_flags::LOOPBACK ? "L" : "-");
  result += (intf.status() & ubench::net::if_flags::MULTICAST ? "M" : "-");
  result += (intf.status() & ubench::net::if_flags::NOARP ? "-" : "A");
  return result;
}

}  // namespace

auto main() -> int {
  auto interfaces = ubench::net::query_net_interfaces();

  // The first column has a dynamic width.
  int ifwidth{10};
  for (const auto& [name, interface] : interfaces) {
    if (interface.alias()) {
      ifwidth =
          // Clang 19.1.4 FALSE POSITIVE.
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          std::max(ifwidth, static_cast<int>(interface.alias()->length()));
    } else {
      ifwidth = std::max(ifwidth, static_cast<int>(name.length()));
    }
  }

  // Column header.
  std::cout << std::left;
  std::cout << std::setw(ifwidth + 1) << "Interface";
  std::cout << std::setw(wid_mac + 1) << "HW Address";
  std::cout << std::setw(wid_mtu + 1) << "MTU";
  std::cout << std::setw(wid_vlan + 1) << "VLAN";
  std::cout << std::setw(ifwidth + 1) << "Parent";
  std::cout << std::setw(wid_type + 1) << "Type";
  std::cout << std::setw(wid_ipv4 + 1) << "Address";
  std::cout << std::setw(wid_ipv4 + 1) << "Net Mask";
  std::cout << std::setw(wid_ipv4 + 1) << "Broadcast";
  std::cout << std::setw(wid_ipv4 + 1) << "Destination";
  std::cout << "UuRLMA" << std::endl;

  std::cout << std::setfill('-');
  std::cout << std::setw(ifwidth) << ""
            << " ";
  std::cout << std::setw(wid_mac) << ""
            << " ";
  std::cout << std::setw(wid_mtu) << ""
            << " ";
  std::cout << std::setw(wid_vlan) << ""
            << " ";
  std::cout << std::setw(ifwidth) << ""
            << " ";
  std::cout << std::setw(wid_type) << ""
            << " ";
  std::cout << std::setw(wid_ipv4) << ""
            << " ";
  std::cout << std::setw(wid_ipv4) << ""
            << " ";
  std::cout << std::setw(wid_ipv4) << ""
            << " ";
  std::cout << std::setw(wid_ipv4) << ""
            << " ";
  std::cout << std::setfill(' ');
  std::cout << "------" << std::endl;

  // Print details of the interfaces.
  for (const auto& [name, interface] : interfaces) {
    if (!interface.inet().empty()) {
      for (const auto& ipv4 : interface.inet()) {
        std::cout << if_details{ifwidth, interface};
        std::cout << ipv4_details(ipv4);
        std::cout << status(interface) << std::endl;
      }
    } else {
      std::cout << if_details{ifwidth, interface};
      std::cout << ipv4_details();
      std::cout << status(interface) << std::endl;
    }
  }
  return 0;
}
