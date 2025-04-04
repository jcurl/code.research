#ifndef POC_BPF_BPF_L2_UDP_PACKET_H
#define POC_BPF_BPF_L2_UDP_PACKET_H

#include "ubench/net.h"
#include "bpf_socket.h"

class packet_view {
 public:
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  packet_view(std::byte* data, std::size_t hdr_size, std::size_t payload_len)
      : data_{data}, hdr_size_{hdr_size}, payload_len_{payload_len} {}
  packet_view(const packet_view&) = default;
  auto operator=(const packet_view&) -> packet_view& = default;
  packet_view(packet_view&&) = default;
  auto operator=(packet_view&&) -> packet_view& = default;
  ~packet_view() = default;

  [[nodiscard]] auto data() const -> const std::byte* { return data_; }

  [[nodiscard]] auto hdr_size() const -> std::size_t { return hdr_size_; }

  [[nodiscard]] auto payload_len() const -> std::size_t { return payload_len_; }

 private:
  std::byte* data_{};
  std::size_t hdr_size_{};
  std::size_t payload_len_{};
};

// clang-format off

// Ethernet + IPv4 Prototype header
//
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  0 | DEST MAC (6 bytes)                                            |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  4 |                               | SOURCE MAC (6 bytes)          |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  8 |                                                               |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// VLAN Prototype - The OS does that for us here.
//
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  0 | Proto 0x8100 VLAN             |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// VLAN Header - The OS does that for us here.
//
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  0 |             TPID              | PCP | |          VID          |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// IPv4 - Prototype
//
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  0 | Proto 0x0800 IPv4             |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// IPv4 - We don't use options.
//
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  0 |Version|  IHL  |Type of Service|          Total Length         |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  4 |         Identification        |Flags|      Fragment Offset    |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  8 |  Time to Live |    Protocol   |         Header Checksum       |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 12 |                       Source Address                          |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 16 |                    Destination Address                        |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// UDP
//
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  0 |          Source Port          |       Destination Port        |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  4 |            Length             |           Checksum            |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// clang-format on

class bpf_socket::l2_udp_packet {
 public:
  l2_udp_packet();
  l2_udp_packet(const l2_udp_packet&) = default;
  auto operator=(const l2_udp_packet&) -> l2_udp_packet& = default;
  l2_udp_packet(l2_udp_packet&&) = default;
  auto operator=(l2_udp_packet&&) -> l2_udp_packet& = default;
  ~l2_udp_packet() = default;

  auto write_src_mac(const ubench::net::ether_addr& mac) -> void;

  auto write_dest_mac(const ubench::net::ether_addr& mac) -> void;

  auto write_src_ip(const sockaddr_in& src) -> void;

  auto write_dest_ip(const sockaddr_in& dest) -> void;

  /// @brief Get the header for a non-fragmented packet.
  auto get_header(const std::vector<std::byte>& udp_payload)
      -> const packet_view;

  /// @brief Get the header for a fragmented packet.
  ///
  /// Calculate the header for the udp_payload, from the offset within the
  /// udp_payload and the length of the udp_payload. The header is assumed that
  /// fragmentation is set. Lengths are checked if this is the last packet or
  /// not. The fragmentation offset is not updated, a separate call is needed.
  auto get_header(const std::vector<std::byte>& udp_payload, std::size_t offset,
      std::uint16_t mtu) -> const packet_view;

  auto increment_fragment_id() -> void { fragmentation_id_++; }

 private:
  std::vector<std::byte> hdr_{};
  std::uint16_t fragmentation_id_{0};
};

#endif