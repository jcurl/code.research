#ifndef UBENCH_NET_H
#define UBENCH_NET_H

#include <sys/socket.h>
#include <arpa/inet.h>

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace ubench::net {

/// @brief Representation of an Ethernet address.
///
/// Do not assume that this structure is compatible with similar structures that
/// your operating system may provide. Not all Operating Systems provide such a
/// public structure.
struct ether_addr {
  ether_addr() = default;
  ether_addr(const ether_addr&) noexcept = default;
  auto operator=(const ether_addr&) noexcept -> ether_addr& = default;
  ether_addr(ether_addr&&) noexcept = default;
  auto operator=(ether_addr&&) noexcept -> ether_addr& = default;

  /// @brief The size, in bytes, of the Ethernet address.
  static constexpr int ETH_ADDR_LEN = 6;

  /// @brief Individual bytes of the Ethernet address, with element 0 being
  /// the first byte (high order).
  std::array<std::uint8_t, ETH_ADDR_LEN> ether_addr_octet{};
};

/// @brief Check if the IPv4 address provided is multicast.
///
/// Given an IPv4 address, checks the bits if this is a multicast address.
///
/// @param addr the address to test. This is already in network byte order.
///
/// @return If there is an error (such as in invalid input) returns no result.
/// Otherwise returns true if the address is an IPv4 multicast address.
auto is_multicast(const in_addr& addr) -> bool;

/// @brief Convert an IPv4 address to a multicast Ethernet address.
///
/// Given a multicast IPv4 address, calculate the associated multicast Ethernet
/// address.
///
/// @param addr the IPv4 address to convert from. This is already in network
/// byte order.
///
/// @param[out] eth the Ethernet MAC address to convert to.
///
/// @return true if the conversion was successful, false otherwise (e.g. the
/// value of addr is not a multicast address, or eth is nullptr).
auto get_ether_multicast(const in_addr& addr, ether_addr* eth) -> bool;

/// @brief Converts addr to a string.
///
/// This is functionality equivalent to inet_ntop() for C++ (can't use this
/// function name as some systems won't compile if the names overlap).
///
/// @param addr The IPv4 address and port to stringify.
///
/// @return The string converted, or on error, an empty string.
auto inet_ntos(const sockaddr_in& addr) -> const std::string;

/// @brief Converts addr to a string.
///
/// This is functionality equivalent to inet_ntop() for C++ (can't use this
/// function name as some systems won't compile if the names overlap).
///
/// @param addr The IPv4 address to stringify.
///
/// @return The string converted, or on error, an empty string.
auto inet_ntos(const in_addr& addr) -> const std::string;

/// @brief Converts addr to a string.
///
/// This is functionality equivalent to eth_ntoa().
///
/// @param addr The Ethernet address to stringify.
///
/// @return The string converted, or on error, an empty string.
auto ether_ntos(const ether_addr& addr) -> const std::string;

/// @brief Interface flags
enum class if_flags {
  UP,           //< Interface is up.
  BROADCAST,    //< Interface supports broadcasting.
  LOOPBACK,     //< Interface is a loopback device.
  POINTOPOINT,  //< Interface is a point-to-point connection.
  RUNNING,      //< Interface is running.
  MULTICAST,    //< Interface supports multicast.
  LOWER_UP,     //< Interface is enabled by the OS.
};

/// @brief Describes an IPv4 configuration of an interface.
class if_ipv4 {
 public:
  /// @brief Constructs this object with IPv4 Address information.
  ///
  /// @param addr the IPv4 address of the interface. The value is copied.
  ///
  /// @param nmask the network mast of the interface, nullptr if not available.
  /// The value is copied.
  ///
  /// @param baddr the broadcast address of the interface, nullptr if not
  /// available. The value is copied.
  ///
  /// @param daddr the destination address of the interface, nullptr if not
  /// available. The value is copied.
  if_ipv4(const in_addr& addr, const in_addr* nmask, const in_addr* baddr,
      const in_addr* daddr) noexcept
      : addr_{addr},
        baddr_{baddr ? std::optional<const in_addr>{*baddr} : std::nullopt},
        daddr_{daddr ? std::optional<const in_addr>{*daddr} : std::nullopt},
        nmask_{nmask ? std::optional<const in_addr>{*nmask} : std::nullopt} {}

  if_ipv4() noexcept = default;
  if_ipv4(const if_ipv4&) noexcept = default;
  auto operator=(const if_ipv4&) noexcept -> if_ipv4& = default;
  if_ipv4(if_ipv4&&) noexcept = default;
  auto operator=(if_ipv4&&) noexcept -> if_ipv4& = default;

  /// @brief Get the IPv4 address assigned ot the interface.
  ///
  /// @return the IPv4 address assigned to the interface (the lifetime is the
  /// same as this object).
  auto addr() const noexcept -> const in_addr& { return addr_; }

  /// @brief Get the IPv4 broadcast address for the network.
  ///
  /// The network broadcast address for bus networks, like 802.1 Ethernet. This
  /// is only set if the flags have if_flags::IFF_BROADCAST.
  ///
  /// @return the IPv4 broadcast address (the lifetime is the same as this
  /// object), nullptr if not available.
  auto broadcast_addr() const noexcept -> const in_addr* {
    return baddr_ ? &baddr_.value() : nullptr;
  }

  /// @brief Get the IPv4 destination address for the interface.
  ///
  /// The destination address is useful for point-to-point networks, like PPP.
  /// This is only set if the flags have if_flags::IFF_POINTOPOINT.
  ///
  /// @return the IPv4 destination address for the interface (the lifetime is
  /// the same as this object), or nullptr if not available.
  auto dest_addr() const noexcept -> const in_addr* {
    return daddr_ ? &daddr_.value() : nullptr;
  }

  /// @brief Get the IPv4 network mask for the interface.
  ///
  /// The network mask is in IPv4 format, Count the number of starting bits to
  /// get the network mask in the `x.x.x.x/N` notation, where `N` is the number
  /// of bits from the left that are set.
  ///
  /// @return the IPv4 network mask for the interface (the lifetime is the same
  /// as this object), or nullptr if not available.
  auto netmask() const noexcept -> const in_addr* {
    return nmask_ ? &nmask_.value() : nullptr;
  }

 private:
  in_addr addr_{};
  std::optional<in_addr> baddr_{};
  std::optional<in_addr> daddr_{};
  std::optional<in_addr> nmask_{};
};

/// @brief A network interface.
class net_if {
 public:
  net_if() = default;
  net_if(const net_if&) noexcept = default;
  auto operator=(const net_if&) noexcept -> net_if& = default;
  net_if(net_if&&) noexcept = default;
  auto operator=(net_if&&) noexcept -> net_if& = default;

  /// @brief Get the name of the interface.
  ///
  /// @return the name of the interface.
  auto name() const noexcept -> const std::string& { return name_; }

  /// @brief Gets an alias to the name of the interface.
  ///
  /// @return the alias to the interface, or nullptr if not available.
  auto alias_name() const noexcept -> const std::string* {
    return alias_ ? &alias_.value() : nullptr;
  }

  /// @brief Get the hardware address of the interface.
  ///
  /// @return the hardware address of the interface, or nullptr if not
  /// available.
  auto hw_addr() const noexcept -> const ether_addr* {
    return hwaddr_ ? &hwaddr_.value() : nullptr;
  }

  /// @brief Get the list of IPv4 addresses assigned to the interface.
  ///
  /// @return the list of IPv4 addresses assigned to the interface.
  auto inet() const noexcept -> const std::vector<if_ipv4>& { return inet_; }

  /// @brief Get the flags for the interface.
  ///
  /// This value is queried from the Operating System on every call.
  ///
  /// @return the flags for the interface. If there is a problem, no flags are
  /// set.
  auto status() const noexcept -> if_flags;

 private:
  std::string name_{};
  std::optional<std::string> alias_{};
  std::optional<ether_addr> hwaddr_{};
  std::vector<if_ipv4> inet_{};

  friend auto query_net_interfaces() -> std::map<std::string, net_if>;
};

/// @brief Query the Operating System for all interfaces and the IP addresses
/// assigned to the interfaces.
///
/// On invocation of this function, all interfaces are queried and returned.
///
/// @return A list of all interfaces and their details.
auto query_net_interfaces() -> std::map<std::string, net_if>;

/// @brief Query a specific interface.
///
/// @param interface is the name of the interface to query.
///
/// @return A pointer to the interface, or empty if the interface isn't
/// available.
auto query_net_interface(std::string_view interface) -> std::unique_ptr<net_if>;

}  // namespace ubench::net

/// @brief Write to the string representation of the IPv4 address and port.
///
/// @param os the output stream.
///
/// @param addr The address to print.
///
/// @return the stream.
auto operator<<(std::ostream& os, const sockaddr_in& addr) -> std::ostream&;

/// @brief Write to the string representation of the IPv4 address.
///
/// @param os the output stream.
///
/// @param addr The address to print.
///
/// @return the stream.
auto operator<<(std::ostream& os, const in_addr& addr) -> std::ostream&;

/// @brief Write to the string representation of the address.
///
/// @param os the output stream.
///
/// @param addr The address to print.
///
/// @return the stream.
auto operator<<(std::ostream& os, const ubench::net::ether_addr& addr)
    -> std::ostream&;

#endif
