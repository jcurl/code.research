#include "bpf_l2_udp_packet.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace {

static constexpr std::size_t PKTHDR_SIZE = 42;

static constexpr std::uint16_t ETH_DST_MAC = 0;
static constexpr std::uint16_t ETH_SRC_MAC = 6;
static constexpr std::uint16_t ETH_PROTO = 12;

static constexpr std::uint16_t ETH_PROTO_IPv4 = 0x0800;

static constexpr std::uint16_t IPV4_PROTO_UDP = 17;
static constexpr std::uint8_t IPV4_TTL = 1;

static constexpr std::size_t IPV4_OFFSET = ETH_PROTO + sizeof(ETH_PROTO_IPv4);
static constexpr std::size_t IPV4_HDR_LEN = 20;
static constexpr std::uint16_t IPV4HDR_VER_IHL = 0;
static constexpr std::uint16_t IPV4HDR_TOS = 1;
static constexpr std::uint16_t IPV4HDR_LEN = 2;
static constexpr std::uint16_t IPV4HDR_ID = 4;
static constexpr std::uint16_t IPV4HDR_FLAGS = 6;
static constexpr std::uint16_t IPV4HDR_TTL = 8;
static constexpr std::uint16_t IPV4HDR_PROTO = 9;
static constexpr std::uint16_t IPV4HDR_CHECKSUM = 10;
static constexpr std::uint16_t IPV4HDR_SRC_IP = 12;
static constexpr std::uint16_t IPV4HDR_DST_IP = 16;

static constexpr std::size_t UDP_OFFSET = IPV4_OFFSET + IPV4_HDR_LEN;
static constexpr std::uint16_t UDP_HDR_LEN = 8;
static constexpr std::uint16_t UDP_SRC_PORT = 0;
static constexpr std::uint16_t UDP_DEST_PORT = 2;
static constexpr std::uint16_t UDP_LEN = 4;
static constexpr std::uint16_t UDP_CHECKSUM = 6;

inline auto read_uint16_t_be(
    const std::vector<std::byte>& data, std::size_t offset) -> std::uint16_t {
  return (static_cast<std::uint16_t>(data[offset]) << 8) |
         static_cast<std::uint16_t>(data[offset + 1]);
}

inline auto read_uint8_t(const std::vector<std::byte>& data, std::size_t offset)
    -> std::uint8_t {
  return (static_cast<std::uint8_t>(data[offset]));
}

inline auto write_uint16_t_be(std::vector<std::byte>& data, std::size_t offset,
    std::uint16_t value) noexcept -> void {
  data[offset] = static_cast<std::byte>((value >> 8) & 0xFF);
  data[offset + 1] = static_cast<std::byte>(value & 0xFF);
}

inline auto write_uint32_t(std::vector<std::byte>& data, std::size_t offset,
    std::uint32_t value) noexcept -> void {
  std::memcpy(&data[offset], &value, sizeof(value));
}

inline auto write_uint16_t(std::vector<std::byte>& data, std::size_t offset,
    std::uint16_t value) noexcept -> void {
  std::memcpy(&data[offset], &value, sizeof(value));
}

inline auto write_uint8_t(std::vector<std::byte>& data, std::size_t offset,
    std::uint8_t value) noexcept -> void {
  data[offset] = static_cast<std::byte>(value);
}

inline auto calc_ipv4_cs(const std::vector<std::byte>& hdr,
    std::size_t offset) noexcept -> std::uint16_t {
  std::uint32_t cs = 0;
  for (std::size_t w = 0; w < IPV4_HDR_LEN / 2; ++w) {
    cs += read_uint16_t_be(hdr, offset + w * 2);
  }
  while (cs > 0xFFFF) {
    cs = (cs & 0xFFFF) + (cs >> 16);
  }
  return ~cs;
}

inline auto calc_udp4_cs(const std::vector<std::byte>& hdr, std::size_t offset,
    const std::vector<std::byte>& payload) noexcept -> std::uint16_t {
  std::uint32_t cs = 0;

  // https://www.ietf.org/rfc/rfc768.txt
  //
  // Checksum is the 16-bit one's complement of the one's complement sum of a
  // pseudo header of information from the IP header, the UDP header, and the
  // data,  padded  with zero octets  at the end (if  necessary)  to  make  a
  // multiple of two octets.
  //
  // The pseudo  header  conceptually prefixed to the UDP header contains the
  // source  address,  the destination  address,  the protocol,  and the  UDP
  // length.   This information gives protection against misrouted datagrams.
  // This checksum procedure is the same as is used in TCP.
  //
  //                   0      7 8     15 16    23 24    31
  //                  +--------+--------+--------+--------+
  //                  |          source address           |
  //                  +--------+--------+--------+--------+
  //                  |        destination address        |
  //                  +--------+--------+--------+--------+
  //                  |  zero  |protocol|   UDP length    |
  //                  +--------+--------+--------+--------+
  //
  // If the computed  checksum  is zero,  it is transmitted  as all ones (the
  // equivalent  in one's complement  arithmetic).   An all zero  transmitted
  // checksum  value means that the transmitter  generated  no checksum  (for
  // debugging or for higher level protocols that don't care).

  // IPv4 pseudo header.
  auto udp_len = static_cast<std::uint16_t>(payload.size()) + UDP_HDR_LEN;
  cs += read_uint16_t_be(hdr, offset + IPV4HDR_SRC_IP);
  cs += read_uint16_t_be(hdr, offset + IPV4HDR_SRC_IP + 2);
  cs += read_uint16_t_be(hdr, offset + IPV4HDR_DST_IP);
  cs += read_uint16_t_be(hdr, offset + IPV4HDR_DST_IP + 2);
  cs += read_uint8_t(hdr, offset + IPV4HDR_PROTO);
  cs += udp_len;

  // Take the UDP Header.
  for (std::size_t w = 0; w < 4; ++w) {
    cs += read_uint16_t_be(hdr, offset + IPV4_HDR_LEN + w * 2);
  }

  // Take the UDP Payload.
  for (std::size_t w = 0; w < (payload.size()) / 2; ++w) {
    cs += read_uint16_t_be(payload, w * 2);
  }

  // If the payload is an odd number of bytes, pad to an even number of bytes.
  if (udp_len % 2 == 1) {
    cs += static_cast<std::uint16_t>(read_uint8_t(payload, payload.size() - 1))
          << 8;
  }

  while (cs > 0xFFFF) {
    cs = (cs & 0xFFFF) + (cs >> 16);
  }

  // In case the result is zero, send a ones' complement.
  if (cs == 0xFFFF) return cs;
  return ~cs;
}

}  // namespace

bpf_socket::l2_udp_packet::l2_udp_packet() {
  // Only the Eth header (12) + Proto (2) + IPv4 header (20) + UDP header (8).
  // No VLAN header.
  hdr_.resize(PKTHDR_SIZE);

  // This code assumes no VLAN header. For BPF, this is added by the OS.
  write_uint16_t_be(hdr_, ETH_PROTO, ETH_PROTO_IPv4);

  // IPv4 Header.
  write_uint8_t(
      hdr_, IPV4_OFFSET + IPV4HDR_VER_IHL, 0x40 | (IPV4_HDR_LEN >> 2));
  write_uint8_t(hdr_, IPV4_OFFSET + IPV4HDR_TOS, 0);
  write_uint8_t(hdr_, IPV4_OFFSET + IPV4HDR_TTL, IPV4_TTL);
  write_uint8_t(hdr_, IPV4_OFFSET + IPV4HDR_PROTO, IPV4_PROTO_UDP);
}

auto bpf_socket::l2_udp_packet::write_src_mac(
    const ubench::net::ether_addr& mac) -> void {
  std::memcpy(&hdr_[ETH_SRC_MAC], &mac.ether_addr_octet[0],
      mac.ether_addr_octet.size());
};

auto bpf_socket::l2_udp_packet::write_dest_mac(
    const ubench::net::ether_addr& mac) -> void {
  std::memcpy(&hdr_[ETH_DST_MAC], &mac.ether_addr_octet[0],
      mac.ether_addr_octet.size());
};

auto bpf_socket::l2_udp_packet::write_src_ip(const sockaddr_in& src) -> void {
  write_uint32_t(hdr_, IPV4_OFFSET + IPV4HDR_SRC_IP, src.sin_addr.s_addr);
  write_uint16_t(hdr_, UDP_OFFSET + UDP_SRC_PORT, src.sin_port);
}

auto bpf_socket::l2_udp_packet::write_dest_ip(const sockaddr_in& dest) -> void {
  write_uint32_t(hdr_, IPV4_OFFSET + IPV4HDR_DST_IP, dest.sin_addr.s_addr);
  write_uint16_t(hdr_, UDP_OFFSET + UDP_DEST_PORT, dest.sin_port);
}

auto bpf_socket::l2_udp_packet::get_header(
    const std::vector<std::byte>& udp_payload) -> const packet_view {
  write_uint16_t_be(hdr_, IPV4_OFFSET + IPV4HDR_LEN,
      udp_payload.size() + IPV4_HDR_LEN + UDP_HDR_LEN);
  write_uint16_t_be(hdr_, IPV4_OFFSET + IPV4HDR_ID, fragmentation_id_);
  write_uint16_t_be(
      hdr_, IPV4_OFFSET + IPV4HDR_FLAGS, 0);  // DF=1, MF=0, Offset = 0
  write_uint16_t_be(
      hdr_, IPV4_OFFSET + IPV4HDR_CHECKSUM, 0);  // Must be zero first.
  auto ip4_cs = calc_ipv4_cs(hdr_, IPV4_OFFSET);
  write_uint16_t_be(hdr_, IPV4_OFFSET + IPV4HDR_CHECKSUM, ip4_cs);

  write_uint16_t_be(hdr_, UDP_OFFSET + UDP_LEN,
      static_cast<std::uint16_t>(udp_payload.size()) + UDP_HDR_LEN);
  write_uint16_t_be(hdr_, UDP_OFFSET + UDP_CHECKSUM, 0);  // Must be zero first.
  auto udp_cs = calc_udp4_cs(hdr_, IPV4_OFFSET, udp_payload);
  write_uint16_t_be(hdr_, UDP_OFFSET + UDP_CHECKSUM, udp_cs);

  return packet_view{hdr_.data(), hdr_.size(), udp_payload.size()};
}

auto bpf_socket::l2_udp_packet::get_header(
    const std::vector<std::byte>& udp_payload,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    std::size_t offset, std::uint16_t mtu) -> const packet_view {
  if (offset == 0) {
    // When sending IPv4 fragmented packets, always start with an offset of zero
    // first. It is undefined behaviour to call this function if the payload is
    // less than the MTU.

    // The fragment offset is multiples of 8, so the length must be a multiple
    // of 8.
    std::uint16_t len = (mtu - IPV4_HDR_LEN - UDP_HDR_LEN) & ~0x7;
    write_uint16_t_be(
        hdr_, IPV4_OFFSET + IPV4HDR_LEN, len + IPV4_HDR_LEN + UDP_HDR_LEN);
    write_uint16_t_be(hdr_, IPV4_OFFSET + IPV4HDR_ID, fragmentation_id_);

    std::uint16_t flags = 0x2000;  // DF=0, MF=1. Offset = 0.
    write_uint16_t_be(hdr_, IPV4_OFFSET + IPV4HDR_FLAGS, flags);

    write_uint16_t_be(hdr_, IPV4_OFFSET + IPV4HDR_CHECKSUM, 0);
    auto ip4_cs = calc_ipv4_cs(hdr_, IPV4_OFFSET);
    write_uint16_t_be(hdr_, IPV4_OFFSET + IPV4HDR_CHECKSUM, ip4_cs);

    // Undefined behaviour (corrupted packets) if the payload size is greater
    // than 65527.
    write_uint16_t_be(hdr_, UDP_OFFSET + UDP_LEN,
        static_cast<std::uint16_t>(udp_payload.size()) + UDP_HDR_LEN);
    write_uint16_t_be(
        hdr_, UDP_OFFSET + UDP_CHECKSUM, 0);  // Must be zero first.
    auto udp_cs = calc_udp4_cs(hdr_, IPV4_OFFSET, udp_payload);
    write_uint16_t_be(hdr_, UDP_OFFSET + UDP_CHECKSUM, udp_cs);

    // User should write the IPv4 + UDP header, then the first 'len' bytes of
    // the payload.
    return packet_view(hdr_.data(), hdr_.size(), len);
  }

  // Subsequent packets will only update fields that are to change from the
  // first packet. Writing the second packet assumes the first packet has
  // already been sent out.

  std::uint16_t len{};
  std::uint16_t flags{};
  // The fragment offset is in units of 8 octets. That implies the maximum
  // length we can send is also multiples of 8 bytes.
  const std::uint16_t max_len = (mtu - IPV4_HDR_LEN) & ~0x7;
  if (udp_payload.size() - offset > max_len) {
    len = max_len;
    flags = 0x2000;
  } else {
    len = udp_payload.size() - offset;
  }
  write_uint16_t_be(hdr_, IPV4_OFFSET + IPV4HDR_LEN, len + IPV4_HDR_LEN);

  // Undefined behaviour (corrupted packets) if the offset is not a multiple of
  // 8.
  flags |= (offset + UDP_HDR_LEN) >> 3;
  write_uint16_t_be(hdr_, IPV4_OFFSET + IPV4HDR_FLAGS, flags);

  write_uint16_t_be(hdr_, IPV4_OFFSET + IPV4HDR_CHECKSUM, 0);
  auto ip4_cs = calc_ipv4_cs(hdr_, IPV4_OFFSET);
  write_uint16_t_be(hdr_, IPV4_OFFSET + IPV4HDR_CHECKSUM, ip4_cs);
  return packet_view{hdr_.data(), IPV4_OFFSET + IPV4_HDR_LEN, len};
}
