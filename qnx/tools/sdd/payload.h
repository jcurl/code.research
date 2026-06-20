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
  payload() = default;
  payload(const payload&) = delete;
  auto operator=(const payload&) -> payload& = delete;
  payload(payload&&) noexcept = default;
  auto operator=(payload&&) noexcept -> payload& = default;
  ~payload() = default;

  /// @brief Open a UDP socket channel.
  ///
  /// @param bind The source address to bind to. If this is INADDR_ANY, we
  /// create one socket per IP address found for all interfaces. Otherwise we
  /// bind to the specific IP address given. Attempt to join the multicast group
  /// on each.
  ///
  /// @param dest The destination address to send to. The address must be a
  /// multicast destination address.
  ///
  /// @return The number of interfaces generated, or the error if unexpected.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  auto open(const sockaddr_in& bind, sockaddr_in dest)
      -> stdext::expected<int, int>;

  /// @brief Generates and sends the payload.
  ///
  /// Sends from the source IPs to the destination multicast specified. This is
  /// done for all interfaces.
  ///
  /// @return The errno, if unexpected.
  auto send() -> stdext::expected<void, int>;

 private:
  auto generate() -> std::string;

  struct iface_ctx {
    unsigned int mtu_;
    unsigned int ipv4hdr_;
    ubench::net::udp udp_;
  };

  std::vector<iface_ctx> sockets_{};
  sockaddr_in dest_{};
};

#endif
