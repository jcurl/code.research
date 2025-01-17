#ifndef UBENCH_NET_COMMON_H
#define UBENCH_NET_COMMON_H

#include <map>
#include <optional>
#include <string>

#include "ubench/file.h"
#include "ubench/net.h"

namespace ubench::net {

/// @brief Use getifaddrs() to query all interfaces and their addresses.
///
/// Works on Cygwin, QNX 7.1, QNX 8.0, Linux, NetBSD, FreeBSD.
///
/// @param interfaces The map to update on output. Existing elements are not
/// removed.
auto query_net_interfaces_getifaddrs(
    std::map<std::string, interface>& interfaces) -> void;

/// @brief Query the friendly name (alias) for a given interface name.
///
/// Works on Cygwin (Windows).
///
/// @param sock A file descriptor for a socket.
///
/// @param interface The name of the interface to query.
///
/// @return The friendly name, or std::nullopt if not available.
[[nodiscard]] auto query_net_interface_friendly_name(
    const ubench::file::fdesc& sock, const std::string& interface)
    -> std::optional<std::string>;

/// @brief Query the Ethernet MAC address for a given interface name.
///
/// Works on Cygwin (Windows), Linux.
///
/// @param sock A file descriptor for a socket.
///
/// @param interface The name of the interface to query.
///
/// @return The Ethernet MAC address, or std::nullopt if not available.
[[nodiscard]] auto query_net_interface_hw_addr(const ubench::file::fdesc& sock,
    const std::string& interface) -> std::optional<ether_addr>;

/// @brief Query the interface VLAN settings.
///
/// Works on Linux, NetBSD, FreeBSD, QNX 7.1, QNX 8.0.
///
/// @param sock A file descriptor for a socket.
///
/// @param interface The name of the interface to query.
///
/// @return The configuration of the VLAN, or std::nullopt if not available.
[[nodiscard]] auto query_net_interface_vlan_id(const ubench::file::fdesc& sock,
    const std::string& interface) -> std::optional<if_vlan>;

/// @brief Query the status flags for the interface.
///
/// @param sock A file descriptor for a socket.
///
/// @param interface The name of the interface to query.
///
/// @return The flags for the interface as returned by the Operating System.
[[nodiscard]] auto query_net_interface_flags(const ubench::file::fdesc& sock,
    const std::string& interface) -> std::optional<unsigned int>;

/// @brief Query the maximum transmission unit for the interface.
///
/// @param sock A file descriptor for a socket.
///
/// @param interface The name of the interface to query.
///
/// @return The  maximum transmission unit for the interface as returned by the
/// Operating System.
[[nodiscard]] auto query_net_interface_mtu(const ubench::file::fdesc& sock,
    const std::string& interface) -> std::optional<unsigned int>;

}  // namespace ubench::net

#endif
