#ifndef UBENCH_NET_H
#define UBENCH_NET_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#if HAVE_NET_INCLUDE_LINUX_IF_H
#include <linux/if.h>
#endif

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "ubench/flags.h"

namespace ubench::net {

/// @brief Representation of an Ethernet address.
///
/// Do not assume that this structure is compatible with similar structures that
/// your operating system may provide. Not all Operating Systems provide such a
/// public structure.
struct ether_addr {
  ether_addr() = default;
  ether_addr(const ether_addr&) = default;
  auto operator=(const ether_addr&) -> ether_addr& = default;
  ether_addr(ether_addr&&) = default;
  auto operator=(ether_addr&&) -> ether_addr& = default;
  ~ether_addr() = default;

  /// @brief The size, in bytes, of the Ethernet address.
  static constexpr int ETH_ADDR_LEN = 6;

  /// @brief Individual bytes of the Ethernet address, with element 0 being
  /// the first byte (high order).
  std::array<std::uint8_t, ETH_ADDR_LEN> ether_addr_octet{};

  /// @brief Test equality for the ether_addr struct.
  ///
  /// @param rhs the struct to compare against.
  ///
  /// @return true if equal, false otherwise.
  auto operator==(const ether_addr& rhs) const -> bool {
    return ether_addr_octet == rhs.ether_addr_octet;
  }

  /// @brief Test inequality for the ether_addr struct.
  ///
  /// @param rhs the struct to compare against.
  ///
  /// @return true if equal, false otherwise.
  auto operator!=(const ether_addr& rhs) const -> bool {
    return !operator==(rhs);
  }
};

/// @brief Check if the IPv4 address provided is multicast.
///
/// Given an IPv4 address, checks the bits if this is a multicast address.
///
/// @param addr the address to test. This is already in network byte order.
///
/// @return If there is an error (such as in invalid input) returns no result.
/// Otherwise returns true if the address is an IPv4 multicast address.
[[nodiscard]] auto is_multicast(const in_addr& addr) -> bool;

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
[[nodiscard]] auto get_ether_multicast(const in_addr& addr, ether_addr* eth)
    -> bool;

/// @brief Converts addr to a string.
///
/// This is functionality equivalent to inet_ntop() for C++ (can't use this
/// function name as some systems won't compile if the names overlap).
///
/// @param addr The IPv4 address and port to stringify.
///
/// @return The string converted, or on error, an empty string.
[[nodiscard]] auto inet_ntos(const sockaddr_in& addr) -> std::string;

/// @brief Converts addr to a string.
///
/// This is functionality equivalent to inet_ntop() for C++ (can't use this
/// function name as some systems won't compile if the names overlap).
///
/// @param addr The IPv4 address to stringify.
///
/// @return The string converted, or on error, an empty string.
[[nodiscard]] auto inet_ntos(const in_addr& addr) -> std::string;

/// @brief Converts addr to a string.
///
/// This is functionality equivalent to eth_ntoa().
///
/// @param addr The Ethernet address to stringify.
///
/// @return The string converted, or on error, an empty string.
[[nodiscard]] auto ether_ntos(const ether_addr& addr) -> std::string;

/// @brief Interface flags.
///
/// The actual value of each flag is independent of the underlying Operating
/// System constants. Not all flags are available everywhere.
///
/// IFF_LOWER_UP is defined on FreeBSD, but not provided by this implementation
/// (netlink sockets and native messaging is not implemented for FreeBSD). It is
/// not present on NetBSD and QNX 7.1. It is present on QNX 8.0, but not tested,
/// expected to be similar to FreeBSD. It is working on Linux and Cygwin.
enum class if_flags : std::uint32_t {
  UP = 0x0001,           //< Interface is up.
  BROADCAST = 0x0002,    //< Interface supports broadcasting.
  LOOPBACK = 0x0008,     //< Interface is a loopback device.
  POINTOPOINT = 0x0010,  //< Interface is a point-to-point connection.
  RUNNING = 0x0040,      //< Interface is running.
  NOARP = 0x0080,        //< Interface requires no address resolution protocol.
  MULTICAST = 0x1000,    //< Interface supports multicast.
  LOWER_UP = 0x10000,    //< Interface is enabled by the OS.
};
DEFINE_GLOBAL_FLAG_OPERATORS(if_flags);

/// @brief Describes an IPv4 configuration of an interface.
class if_ipv4 {
 public:
  /// @brief Constructs this object with IPv4 Address information.
  ///
  /// Sets the IPv4 address of this object that is immutable. The other
  /// properties can be set and modified later.
  ///
  /// @param addr the IPv4 address of the interface. The value is copied.
  if_ipv4(const in_addr& addr) noexcept : addr_{addr} {}

  if_ipv4(const if_ipv4&) = default;
  auto operator=(const if_ipv4&) -> if_ipv4& = default;
  if_ipv4(if_ipv4&&) = default;
  auto operator=(if_ipv4&&) -> if_ipv4& = default;
  ~if_ipv4() = default;

  /// @brief Get the IPv4 address assigned ot the interface.
  ///
  /// @return the IPv4 address assigned to the interface.
  [[nodiscard]] auto addr() const noexcept -> const in_addr& { return addr_; }

  /// @brief Get the IPv4 broadcast address for the network.
  ///
  /// The network broadcast address for bus networks, like 802.1 Ethernet. This
  /// is only set if the flags have if_flags::IFF_BROADCAST.
  ///
  /// @return the IPv4 broadcast address (the lifetime is the same as this
  /// object), std::nullopt if not available.
  [[nodiscard]] auto broadcast_addr() const noexcept
      -> const std::optional<in_addr>& {
    return baddr_;
  }

  /// @brief Assign the broadcast address (for non-point-to-point types).
  ///
  /// @param addr The broadcast address to set. If std::nullopt, then the
  /// broadcast address is unset.
  auto broadcast_addr(const std::optional<in_addr> addr) noexcept -> void {
    baddr_ = std::move(addr);
  }

  /// @brief Get the IPv4 destination address for the interface.
  ///
  /// The destination address is useful for point-to-point networks, like PPP.
  /// This is only set if the flags have if_flags::IFF_POINTOPOINT.
  ///
  /// @return the IPv4 destination address for the interface (the lifetime is
  /// the same as this object), or std::nullopt if not available.
  [[nodiscard]] auto dest_addr() const noexcept
      -> const std::optional<in_addr>& {
    return daddr_;
  }

  /// @brief Assign the destination address (for point-to-point types).
  ///
  /// @param addr The destination address to set. If std::nullopt, then the
  /// destination address is unset.
  auto dest_addr(const std::optional<in_addr> addr) noexcept -> void {
    daddr_ = std::move(addr);
  }

  /// @brief Get the IPv4 network mask for the interface.
  ///
  /// The network mask is in IPv4 format, Count the number of starting bits to
  /// get the network mask in the `x.x.x.x/N` notation, where `N` is the number
  /// of bits from the left that are set.
  ///
  /// @return the IPv4 network mask for the interface (the lifetime is the same
  /// as this object), or std::nullopt if not available.
  [[nodiscard]] auto netmask() const noexcept -> const std::optional<in_addr>& {
    return nmask_;
  }

  /// @brief Assign the network mask.
  ///
  /// @param addr The network mast to set. If std::nullopt, then the network
  /// mask is unset.
  auto netmask(const std::optional<in_addr> addr) -> void {
    nmask_ = std::move(addr);
  }

  /// @brief Test equality for the if_ipv4 struct.
  ///
  /// @param rhs the struct to compare against.
  ///
  /// @return true if equal, false otherwise.
  [[nodiscard]] auto operator==(const if_ipv4& rhs) const noexcept -> bool;

  /// @brief Test inequality for the if_ipv4 struct.
  ///
  /// @param rhs the struct to compare against.
  ///
  /// @return true if unequal, false otherwise.
  [[nodiscard]] auto operator!=(const if_ipv4& rhs) const noexcept -> bool {
    return !operator==(rhs);
  }

 private:
  in_addr addr_{};
  std::optional<in_addr> baddr_{};
  std::optional<in_addr> daddr_{};
  std::optional<in_addr> nmask_{};
};

/// @brief Details about VLAN for the interface.
struct if_vlan {
  /// @brief The name of the parent device.
  std::string parent;

  /// @brief THe VLAN identifier (range 0..4095).
  std::uint16_t id;

  // There is generally no priority field, as an application normally provides
  // the QOS priority, and the kernel maps that to the 802.1q priority field
  // before sending out the packet.

  /// @brief Test equality for the ether_addr struct.
  ///
  /// @param rhs the struct to compare against.
  ///
  /// @return true if equal, false otherwise.
  auto operator==(const if_vlan& rhs) const -> bool {
    return parent == rhs.parent && id == rhs.id;
  }

  /// @brief Test inequality for the ether_addr struct.
  ///
  /// @param rhs the struct to compare against.
  ///
  /// @return true if equal, false otherwise.
  auto operator!=(const if_vlan& rhs) const -> bool { return !operator==(rhs); }
};

/// @brief A network interface.
///
/// This class captures the results of a query. Setting properties does not
/// modify the Operating System.
class interface {
 public:
  interface(std::string name) : name_{std::move(name)} {}
  interface(const interface&) = default;
  auto operator=(const interface&) -> interface& = default;
  interface(interface&&) = default;
  auto operator=(interface&&) -> interface& = default;
  ~interface() = default;

  /// @brief Get the name of the interface.
  ///
  /// @return the name of the interface.
  [[nodiscard]] auto name() const noexcept -> const std::string& {
    return name_;
  }

  /// @brief Gets an alias to the name of the interface.
  ///
  /// @return the alias to the interface, or std::nullopt if not available.
  [[nodiscard]] auto alias() const noexcept
      -> const std::optional<std::string>& {
    return alias_;
  }

  /// @brief Sets an alias to this interface.
  ///
  /// @param alias The alias. If std::nullopt then the alias is unset.
  auto alias(std::optional<std::string> alias) noexcept -> void {
    alias_ = std::move(alias);
  }

  /// @brief Get the hardware address of the interface.
  ///
  /// @return the hardware address of the interface, or std::nullopt if not
  /// available.
  [[nodiscard]] auto hw_addr() const noexcept
      -> const std::optional<ether_addr>& {
    return hwaddr_;
  }

  /// @brief Sets the Ethernet hardware address to this interface.
  ///
  /// @param addr The hardware address. If std::nullopt then the hardware
  /// address is unset.
  auto hw_addr(const ether_addr& addr) noexcept -> void { hwaddr_ = addr; }

  /// @brief Sets the Ethernet hardware address to this interface.
  ///
  /// @param addr The hardware address. If std::nullopt then the hardware
  /// address is unset.
  auto hw_addr(std::optional<ether_addr> addr) noexcept -> void {
    hwaddr_ = std::move(addr);
  }

  /// @brief Get the VLAN identifier of the interface.
  ///
  /// The VLAN identifier is in the range of 0 to 4095.
  ///
  /// @return the VLAN identifier, or no value if there is no VLAN assigned (or
  /// not supported).
  [[nodiscard]] auto vlan() const noexcept -> const std::optional<if_vlan>& {
    return vlan_;
  }

  /// @brief Sets the VLAN identifier of the interface.
  ///
  /// @param vlan the VLAN identifier. Set to std::nullopt to indicate no VLAN
  /// is assigned. If the range of the VLAN is invalid according to 802.1q, then
  /// it is assumed no VLAN is assigned.
  [[nodiscard]] auto vlan(std::optional<if_vlan> vlan) noexcept -> bool {
    if (!vlan) {
      vlan_ = std::nullopt;
    } else {
      if (vlan->id > 4095) return false;

      vlan_ = std::move(vlan);
    }
    return true;
  }

  /// @brief Get the Maximum Transmission Unit for the interface.
  ///
  /// @return the maximum transmission unit, in bytes, for the interface.
  [[nodiscard]] auto mtu() const noexcept -> const std::optional<unsigned int> {
    return mtu_;
  }

  /// @brief Set the Maximum Transmission Unit for the interface.
  ///
  /// @param mtu the actual maximum transmission unit for the interface.
  auto mtu(std::optional<unsigned int> mtu) noexcept -> void {
    mtu_ = std::move(mtu);
  }

  /// @brief Get the list of IPv4 addresses assigned to the interface.
  ///
  /// Some Operating Systems (e.g. Linux) only return one address per family per
  /// interface. Some Operating Systems (e.g. NetBSD, QNX) can return multiple
  /// addresses per family for one interface. If in the former case you need to
  /// identify the interfaces, check the name for ":N" (where N is a number) at
  /// the end for the same interface. It might not be enough just to group by
  /// the hw_addr() as some systems (some Solaris machines) might have the same
  /// hardware address assigned to multiple NICs.
  ///
  /// @return the list of IPv4 addresses assigned to the interface.
  [[nodiscard]] auto inet() noexcept -> std::vector<if_ipv4>& { return inet_; }

  /// @brief Get the list of IPv4 addresses assigned to the interface.
  ///
  /// Some Operating Systems (e.g. Linux) only return one address per family per
  /// interface. Some Operating Systems (e.g. NetBSD, QNX) can return multiple
  /// addresses per family for one interface. If in the former case you need to
  /// identify the interfaces, check the name for ":N" (where N is a number) at
  /// the end for the same interface. It might not be enough just to group by
  /// the hw_addr() as some systems (some Solaris machines) might have the same
  /// hardware address assigned to multiple NICs.
  ///
  /// @return the list of IPv4 addresses assigned to the interface.
  [[nodiscard]] auto inet() const noexcept -> const std::vector<if_ipv4>& {
    return inet_;
  }

  /// @brief Get the flags for the interface, as given by the Operating System.
  ///
  /// @return the flags for the interface. If there is a problem, no flags are
  /// set.
  [[nodiscard]] auto flags() const noexcept -> unsigned int { return flags_; }

  /// @brief Set the flags for the interface, as returned by the Operating
  /// System.
  auto flags(unsigned int flags) noexcept -> void { flags_ = flags; }

  /// @brief Gets the flags, but converted to the enumeration.
  ///
  /// This function abstracts the Operating System flags, to a common set
  /// independent of the Operating System.
  ///
  /// @return The flags that are set.
  [[nodiscard]] auto status() const noexcept -> const ubench::flags<if_flags>;

 private:
  std::string name_{};
  std::optional<std::string> alias_{};
  std::optional<ether_addr> hwaddr_{};
  std::optional<if_vlan> vlan_{};
  std::optional<int> mtu_{};
  std::vector<if_ipv4> inet_{};
  unsigned int flags_{};
};

/// @brief Query the Operating System for all interfaces and the IP addresses
/// assigned to the interfaces.
///
/// On invocation of this function, all interfaces are queried and returned.
///
/// @return A list of all interfaces and their details.
[[nodiscard]] auto query_net_interfaces()
    -> const std::map<std::string, interface>;

/// @brief Query a specific interface.
///
/// @param interface is the name of the interface to query.
///
/// @return A pointer to the interface, or empty if the interface isn't
/// available.
[[nodiscard]] auto query_net_interface(std::string name)
    -> const std::optional<interface>;

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
