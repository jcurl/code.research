#ifndef SDD_UDP_H
#define SDD_UDP_H

#include <sys/types.h>
#include <netinet/in.h>

#include <string_view>

#include "stdext/expected.h"
#include "ubench/file.h"

namespace ubench::net {

/// @brief Handles UDP sockets.
class udp {
 public:
  udp() = default;

  udp(const udp& other) = delete;
  auto operator=(const udp& other) -> udp& = delete;
  udp(udp&& other) noexcept = default;
  auto operator=(udp&& other) noexcept -> udp& = default;
  ~udp() = default;

  /// @brief Get the socket send buffer size.
  ///
  /// @return The buffer size, or the errno if unexpected.
  auto get_sendbuf() noexcept -> stdext::expected<int, int>;

  /// @brief Set the socket send buffer size.
  ///
  /// @param buf the new buffer size.
  ///
  /// @return The errno, if unexpected.
  auto set_sendbuf(int buf) noexcept -> stdext::expected<void, int>;

  /// @brief Get if sent multicast packets should be looped back to the local
  /// sockets.
  ///
  /// @return If enabled or disabled, or the errno if unexpected.
  auto get_multicast_loop() noexcept -> stdext::expected<bool, int>;

  /// @brief Set if multicast packets should be looped back to the local
  /// sockets.
  ///
  /// @param enable if it should be enabled or not.
  ///
  /// @return The errno, if unexpected.
  auto set_multicast_loop(bool enable) noexcept -> stdext::expected<void, int>;

  /// @brief Joins the address bind in the constructor as a multicast socket.
  ///
  /// @param local the local address to bind to for joining a multicast group.
  ///
  /// @return The errno, if unexpected.
  auto multicast_join(const sockaddr_in& local) noexcept
      -> stdext::expected<void, int>;

  /// @brief Read the time-to-live value of outgoing multicast packets for this
  /// socket.
  ///
  /// @return The TTL, or the errno if unexpected.
  auto get_multicast_ttl() noexcept -> stdext::expected<int, int>;

  /// @brief Set the time-to-live value of the outgoing multicast packets for
  /// this socket.
  ///
  /// @param ttl the new TTL value.
  ///
  /// @return The errno, if unexpected.
  auto set_multicast_ttl(int ttl) noexcept -> stdext::expected<void, int>;

  /// @brief Read if socket address reuse is enabled for binding.
  ///
  /// @return If reuse is enabled, or the errno if unexpected.
  auto get_reuse_addr() noexcept -> stdext::expected<bool, int>;

  /// @brief Set socket address reuse.
  ///
  /// @param enable if reuse should be enabled.
  ///
  /// @return The errno, if unexpected.
  auto set_reuse_addr(bool enable) noexcept -> stdext::expected<void, int>;

  /// @brief Read if socket port reuse is enabled for binding.
  ///
  /// @return If reuse is enabled, or the errno if unexpected.
  auto get_reuse_port() noexcept -> stdext::expected<bool, int>;

  /// @brief Set port address reuse.
  ///
  /// @param enable if reuse should be enabled.
  ///
  /// @return The errno, if unexpected.
  auto set_reuse_port(bool enable) noexcept -> stdext::expected<void, int>;

  /// @brief Get the size of the IPv4 header.
  ///
  /// Get the size of the IPv4 header, which is normally 20 bytes.
  ///
  /// @return The size of the header, or the errno if unexpected.
  auto ipv4hdr_length() noexcept -> stdext::expected<int, int>;

  /// @brief Open the socket and bind it to the port.
  ///
  /// Equivalent to not binding to any socket.
  ///
  /// @return The errno, if unexpected.
  auto open() noexcept -> stdext::expected<void, int>;

  /// @brief Open the socket and bind it to the port.
  ///
  /// @param bind The local address to bind the socket to. If the socket is
  /// INADDR_ANY, then the socket is not bound.
  ///
  /// @return The errno, if unexpected.
  auto open(const sockaddr_in& bind) noexcept -> stdext::expected<void, int>;

  /// @brief Closes the socket.
  ///
  /// This is the same behaviour as when this class is destroyed.
  auto close() noexcept -> void { socket_.close(); }

  /// @brief Send a UTF-8 sequence of characters as the UDP payload.
  ///
  /// @param dest the destination socket
  ///
  /// @param payload the payload to send as raw bytes.
  ///
  /// @return The number of bytes sent, or the errno if unexpected.
  auto send(const sockaddr_in& dest, std::string_view payload)
      -> stdext::expected<int, int>;

 private:
  ubench::file::fdesc socket_{};
};

}  // namespace ubench::net

#endif
