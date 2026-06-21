#ifndef SDD_PAYLOAD_H
#define SDD_PAYLOAD_H

#include <string>
#include <vector>

#include "ubench/net.h"
#include "udp.h"

// Forward declarations for writing values into JSON. This must be in the public
// header file, so that `payload.cpp` sees the definition before including
// "json_writer.h" for its template expansion.
namespace std {

auto to_string(const ubench::net::if_ipv4& addr) -> std::string;
auto to_string(const ubench::net::ether_addr& addr) -> std::string;

}  // namespace std

/// @brief Class to maintain state for generating a UDP payload for discovery.
class payload {
 public:
  /// @brief Initialises the payload sender with the destination multicast
  /// address.
  ///
  /// @param dest The multicast address to send packets to.
  ///
  /// @exception std::invalid_argument The destination address is not a
  /// multicast IPv4 address.
  payload(const sockaddr_in& dest);

  payload(const payload&) = delete;
  auto operator=(const payload&) -> payload& = delete;
  payload(payload&&) noexcept = default;
  auto operator=(payload&&) noexcept -> payload& = default;
  ~payload() = default;

  /// @brief Query all interfaces and add them to our list.
  ///
  /// @param srcport The source port to bind to.
  ///
  /// @return true if there is at least one interface transmitting.
  auto query(in_port_t srcport) -> bool;

  /// @brief Query all IP addresses and add them to our list.
  ///
  /// @param iface The interface to query for.
  ///
  /// @return true if there is at least one interface transmitting.
  auto query(std::string iface, in_port_t srcport) -> bool;

  /// @brief Query the specific IP addresses and add them to our list.
  ///
  /// @param iface The interface to query for.
  ///
  /// @return true if there is at least one interface transmitting.
  auto query(const sockaddr_in& iface) -> bool;

  /// @brief Adds a socket to the list to send a payload to.
  ///
  /// @param bind The address to add.
  ///
  /// @param mtu The MTU of the interface.
  ///
  /// @return The errno, if unexpected.
  auto open(const sockaddr_in& bind, unsigned int mtu)
      -> stdext::expected<void, int>;

  /// @brief Removes a socket from the list to send a payload to.
  ///
  /// @param bind The address to remove.
  ///
  /// @return The errno, if unexpected.
  auto close(const sockaddr_in& bind) -> stdext::expected<void, int>;

  /// @brief Generates and sends the payload.
  ///
  /// Sends from the source IPs to the destination multicast specified. This is
  /// done for all interfaces.
  ///
  /// @return The errno, if unexpected.
  auto send() -> stdext::expected<void, int>;

  /// @brief Get if there is at least one socket defined.
  operator bool() const { return !sockets_.empty(); }

 private:
  auto generate() -> std::string;
  auto set_inactive() -> void;
  auto close_inactive() -> void;

  struct iface_ctx {
    sockaddr_in src{};
    unsigned int mtu{};
    unsigned int ipv4hdr{};
    ubench::net::udp udp{};
    bool active{};
  };

  std::vector<iface_ctx> sockets_{};
  sockaddr_in dest_{};
};

#endif
