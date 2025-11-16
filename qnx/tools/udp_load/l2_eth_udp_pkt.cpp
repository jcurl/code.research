#include "l2_eth_udp_pkt.h"

#include <cstring>

namespace {

constexpr std::uint16_t ETH_DST_MAC = 0;
constexpr std::uint16_t ETH_SRC_MAC = 6;
constexpr std::uint16_t ETH_PROTO = 12;

constexpr std::uint16_t ETH_PROTO_VLAN = 0x8100;
constexpr std::uint16_t ETH_PROTO_IPv4 = 0x0800;
constexpr std::uint16_t IPV4_PROTO_UDP = 17;

constexpr std::uint16_t IPV4HDR_START = 0;
constexpr std::uint16_t IPV4HDR_VER_IHL = 0;
constexpr std::uint16_t IPV4HDR_TOS = 1;
constexpr std::uint16_t IPV4HDR_LEN = 2;
constexpr std::uint16_t IPV4HDR_ID = 4;
constexpr std::uint16_t IPV4HDR_FLAGS = 6;
constexpr std::uint16_t IPV4HDR_TTL = 8;
constexpr std::uint16_t IPV4HDR_PROTO = 9;
constexpr std::uint16_t IPV4HDR_CHECKSUM = 10;
constexpr std::uint16_t IPV4HDR_SRC_IP = 12;
constexpr std::uint16_t IPV4HDR_DST_IP = 16;

constexpr std::uint16_t UDP_START = 20;
constexpr std::uint16_t UDP_SRC_PORT = 0;
constexpr std::uint16_t UDP_DEST_PORT = 2;
constexpr std::uint16_t UDP_LEN = 4;
constexpr std::uint16_t UDP_CHECKSUM = 6;

constexpr std::uint16_t UDP_HDR_LEN = 8;

inline auto read_uint16_t_be(const std::vector<std::uint8_t>& data,
    std::ptrdiff_t offset) -> std::uint16_t {
  return (data[offset] << 8) | data[offset + 1];
}

inline auto write_uint16_t_be(std::vector<std::uint8_t>& data,
    std::ptrdiff_t offset, std::uint16_t value) -> void {
  data[offset] = (value >> 8) & 0xFF;
  data[offset + 1] = value & 0xFF;
}

inline auto write_uint16_t(std::vector<std::uint8_t>& data,
    std::ptrdiff_t offset, std::uint16_t value) -> void {
  std::memcpy(&data[offset], &value, sizeof(value));
}

inline auto write_uint32_t(std::vector<std::uint8_t>& data,
    std::ptrdiff_t offset, std::uint32_t value) -> void {
  std::memcpy(&data[offset], &value, sizeof(value));
}

auto calc_ipv4_cs(const std::vector<std::uint8_t>& hdr,
    std::ptrdiff_t offset) noexcept -> std::uint16_t {
  std::uint32_t size = (hdr[offset + IPV4HDR_VER_IHL] & 0x0f) << 2;
  std::uint32_t cs = 0;

  for (std::ptrdiff_t w = 0; w < size / 2; ++w) {
    cs += read_uint16_t_be(hdr, offset + w * 2);
  }
  while (cs > 0xFFFF) {
    cs = (cs & 0xFFFF) + (cs >> 16);
  }
  return ~cs;
}

auto calc_udp_cs(const std::vector<std::uint8_t>& hdr, std::ptrdiff_t offset,
    const std::vector<std::uint8_t>& payload) noexcept -> std::uint16_t {
  std::uint16_t ipv4_size = (hdr[offset + IPV4HDR_VER_IHL] & 0x0f) << 2;
  std::uint32_t udp_len = read_uint16_t_be(hdr, offset + ipv4_size + UDP_LEN);

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
  cs += read_uint16_t_be(hdr, offset + IPV4HDR_SRC_IP);
  cs += read_uint16_t_be(hdr, offset + IPV4HDR_SRC_IP + 2);
  cs += read_uint16_t_be(hdr, offset + IPV4HDR_DST_IP);
  cs += read_uint16_t_be(hdr, offset + IPV4HDR_DST_IP + 2);
  cs += hdr[offset + IPV4HDR_PROTO];
  cs += udp_len;

  // Take the UDP Header.
  for (std::ptrdiff_t w = 0; w < 4; ++w) {
    cs += read_uint16_t_be(hdr, offset + ipv4_size + w * 2);
  }

  // Take the UDP Payload.
  for (std::ptrdiff_t w = 0; w < (udp_len - UDP_HDR_LEN) / 2; ++w) {
    cs += read_uint16_t_be(payload, w * 2);
  }

  // If the payload is an odd number of bytes, pad to an even number of bytes.
  if (udp_len % 2 == 1) {
    cs += payload[udp_len - UDP_HDR_LEN - 1] << 8;
  }

  while (cs > 0xFFFF) {
    cs = (cs & 0xFFFF) + (cs >> 16);
  }

  // In case the result is zero, send a ones' complement.
  if (cs == 0xFFFF) return cs;
  return ~cs;
}

}  // namespace

auto l2_eth_udp_pkt::build_pkt_hdr() -> const std::vector<std::uint8_t>& {
  if (reset_) {
    // Calculate the size of the header.
    std::uint16_t pkt_hdr_size = 42 + (vlan_id_ < 4096 ? 4 : 0);
    pkt_hdr_.resize(pkt_hdr_size);

    // Build the packet header based on the data. Create the checksums.
    std::memcpy(&pkt_hdr_[ETH_DST_MAC], &dst_mac_.ether_addr_octet[0],
        dst_mac_.ether_addr_octet.size());
    std::memcpy(&pkt_hdr_[ETH_SRC_MAC], &src_mac_.ether_addr_octet[0],
        src_mac_.ether_addr_octet.size());

    int ipv4_offset = 12;

    // VLAN Header.
    if (vlan_id_ < 4096) {
      // VLAN ID is present.
      write_uint16_t_be(pkt_hdr_, ETH_PROTO, ETH_PROTO_VLAN);
      write_uint16_t_be(pkt_hdr_, ETH_PROTO + 2, vlan_id_);
      ipv4_offset += 4;
    }

    write_uint16_t_be(pkt_hdr_, ipv4_offset, ETH_PROTO_IPv4);
    ipv4_offset += 2;

    // IPv4 Header.
    pkt_hdr_[ipv4_offset + IPV4HDR_VER_IHL] = 0x45;  // IPv4, 20 bytes
    pkt_hdr_[ipv4_offset + IPV4HDR_TOS] = 0;         // Type of service
    write_uint16_t_be(pkt_hdr_, ipv4_offset + IPV4HDR_LEN,
        pkt_data_.size() + 28);  // Total length
    write_uint16_t_be(pkt_hdr_, ipv4_offset + IPV4HDR_ID, fragmentation_id_++);
    write_uint16_t_be(pkt_hdr_, ipv4_offset + IPV4HDR_FLAGS,
        0);  // DF=1, MF=0
    pkt_hdr_[ipv4_offset + IPV4HDR_TTL] = ttl_;
    pkt_hdr_[ipv4_offset + IPV4HDR_PROTO] = IPV4_PROTO_UDP;
    write_uint16_t_be(pkt_hdr_, ipv4_offset + IPV4HDR_CHECKSUM, 0);
    write_uint32_t(pkt_hdr_, ipv4_offset + IPV4HDR_SRC_IP, src_ip_.s_addr);
    write_uint32_t(pkt_hdr_, ipv4_offset + IPV4HDR_DST_IP, dst_ip_.s_addr);

    // UDP Header.
    int udp_offset = ipv4_offset + UDP_START;
    write_uint16_t(pkt_hdr_, udp_offset + UDP_SRC_PORT, src_port_);
    write_uint16_t(pkt_hdr_, udp_offset + UDP_DEST_PORT, dst_port_);
    write_uint16_t_be(
        pkt_hdr_, udp_offset + UDP_LEN, pkt_data_.size() + UDP_HDR_LEN);
    write_uint16_t_be(pkt_hdr_, udp_offset + UDP_CHECKSUM, 0);

    // Calculate checksums.
    write_uint16_t_be(pkt_hdr_, ipv4_offset + IPV4HDR_CHECKSUM,
        calc_ipv4_cs(pkt_hdr_, ipv4_offset + IPV4HDR_START));
    write_uint16_t_be(pkt_hdr_, udp_offset + UDP_CHECKSUM,
        calc_udp_cs(pkt_hdr_, ipv4_offset + IPV4HDR_START, pkt_data_));
    reset_ = false;
  }
  return pkt_hdr_;
}