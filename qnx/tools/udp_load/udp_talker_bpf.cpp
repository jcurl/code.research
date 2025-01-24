#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <net/bpf.h>
#include <net/if.h>
#include <netinet/in.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "ubench/net.h"
#include "udp_talker.h"

namespace {

/// @brief Read a uint16_t from data at offset in big-endian.
///
/// @param data the buffer to read from.
///
/// @param offset the offset to read the uint16_t from.
///
/// @return the value read.
inline auto read_uint16_t_be(const std::vector<std::uint8_t>& data,
    std::ptrdiff_t offset) -> std::uint16_t {
  return (data[offset] << 8) | data[offset + 1];
}

/// @brief Write value to data at offset in big-endian.
///
/// @param data the buffer to write to.
///
/// @param offset the offset into data to write the uint16_t.
///
/// @param value the value to write.
inline auto write_uint16_t_be(std::vector<std::uint8_t>& data,
    std::ptrdiff_t offset, std::uint16_t value) -> void {
  data[offset] = (value >> 8) & 0xFF;
  data[offset + 1] = value & 0xFF;
}

/// @brief Write value to data at offset using the host-endian.
///
/// @param data the buffer to write to.
///
/// @param offset the offset into data to write the uint16_t.
///
/// @param value the value to write.
inline auto write_uint16_t(std::vector<std::uint8_t>& data,
    std::ptrdiff_t offset, std::uint16_t value) -> void {
  std::memcpy(&data[offset], &value, sizeof(value));
}

/// @brief Write value to data at offset using the host-endian.
///
/// @param data the buffer to write to.
///
/// @param offset the offset into data to write the uint32_t.
///
/// @param value the value to write.
inline auto write_uint32_t(std::vector<std::uint8_t>& data,
    std::ptrdiff_t offset, std::uint32_t value) -> void {
  std::memcpy(&data[offset], &value, sizeof(value));
}

}  // namespace

class udp_talker_bpf::pkt_details {
 public:
  pkt_details() = default;
  ~pkt_details() = default;
  pkt_details(const pkt_details&) = delete;
  auto operator=(const pkt_details&) -> pkt_details& = delete;
  pkt_details(pkt_details&& other) = delete;
  auto operator=(pkt_details&&) -> pkt_details& = delete;

  [[nodiscard]] auto dst_mac() noexcept -> ubench::net::ether_addr& { return dst_mac_; }
  [[nodiscard]] auto src_mac() noexcept -> ubench::net::ether_addr& { return src_mac_; }
  [[nodiscard]] auto vlan_id() noexcept -> std::uint16_t& { return vlan_id_; }
  [[nodiscard]] auto fragmentation_id() noexcept -> std::uint16_t& { return fragmentation_id_; }
  [[nodiscard]] auto ttl() noexcept -> std::uint8_t& { return ttl_; }
  [[nodiscard]] auto dst_ip() noexcept -> in_addr& { return dst_ip_; }
  [[nodiscard]] auto src_ip() noexcept -> in_addr& { return src_ip_; }
  [[nodiscard]] auto dst_port() noexcept -> std::uint16_t& { return dst_port_; }
  [[nodiscard]] auto src_port() noexcept -> std::uint16_t& { return src_port_; }

  auto reset_pkt() noexcept -> void { reset_ = true; }

  [[nodiscard]] auto pkt_data() noexcept -> std::vector<std::uint8_t>& { return pkt_data_; }

  /// @brief Build the packet header.
  ///
  /// Call after all fields are assigned. It will rebuild the packet the first
  /// time. If you modify any of the fields after rebuilding, call reset_pkt()
  /// so that this function will rebuild the packet header again (else it just
  /// returns the last calculated value).
  ///
  /// @return the packet header.
  [[nodiscard]] auto build_pkt_hdr() -> const std::vector<std::uint8_t>&;

 private:
  bool reset_{true};
  ubench::net::ether_addr dst_mac_{};
  ubench::net::ether_addr src_mac_{};
  std::uint16_t vlan_id_{65535};
  std::uint16_t fragmentation_id_{0};
  std::uint8_t ttl_{1};
  in_addr dst_ip_{};
  in_addr src_ip_{};
  std::uint16_t dst_port_{};
  std::uint16_t src_port_{};
  std::vector<std::uint8_t> pkt_hdr_{};
  std::vector<std::uint8_t> pkt_data_{};

  static constexpr std::uint16_t ETH_DST_MAC = 0;
  static constexpr std::uint16_t ETH_SRC_MAC = 6;
  static constexpr std::uint16_t ETH_PROTO = 12;

  static constexpr std::uint16_t ETH_PROTO_VLAN = 0x8100;
  static constexpr std::uint16_t ETH_PROTO_IPv4 = 0x0800;
  static constexpr std::uint16_t IPV4_PROTO_UDP = 17;

  static constexpr std::uint16_t IPV4HDR_START = 0;
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

  static constexpr std::uint16_t UDP_START = 20;
  static constexpr std::uint16_t UDP_SRC_PORT = 0;
  static constexpr std::uint16_t UDP_DEST_PORT = 2;
  static constexpr std::uint16_t UDP_LEN = 4;
  static constexpr std::uint16_t UDP_CHECKSUM = 6;

  static constexpr std::uint16_t UDP_HDR_LEN = 8;

  /// @brief Given the start of an IPv4 header, calculate the IPv4 Header
  /// checksum
  ///
  /// The checksum value must be zero prior for the checksum to be calculated
  /// correctly.
  ///
  /// @param hdr the buffer containing the IPv4 header
  ///
  /// @param offset the offset into the buffer hdr where the IPv4 header starts
  ///
  /// @return the IPv4 checksum that can be copied into the checksum field.
  auto calc_ipv4_cs(const std::vector<std::uint8_t>& hdr, std::ptrdiff_t offset)
      noexcept -> std::uint16_t;

  /// @brief Given the IPv4 header for UDP, calculate the UDP checksum
  ///
  /// The checksum is calculated assuming the prototype is UDP (proto 17). The
  /// header is used to get the offset where the UDP data is, which is assumed
  /// to also be in the buffer hdr.
  ///
  /// Ensure that the IPv4 header is correct (has the correct UDP length,
  /// correct header length) and that the payload contains the complete UDP
  /// payload.
  ///
  /// @param hdr the IPv4 header, and the UDP header.
  ///
  /// @param offset the offset into the buffer hdr where the IPv4 data starts.
  ///
  /// @param payload the payload where the UDP starts at position zero.
  ///
  /// @return the UDP checksum that can be copied into the checksum field of the
  /// UDP section stored in hdr.
  auto calc_udp_cs(const std::vector<std::uint8_t>& hdr, std::ptrdiff_t offset,
      const std::vector<std::uint8_t>& payload) noexcept -> std::uint16_t;
};

auto udp_talker_bpf::pkt_details::calc_ipv4_cs(
    const std::vector<std::uint8_t>& hdr, std::ptrdiff_t offset) noexcept
    -> std::uint16_t {
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

auto udp_talker_bpf::pkt_details::calc_udp_cs(
    const std::vector<std::uint8_t>& hdr, std::ptrdiff_t offset,
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
    cs += payload[udp_len - 1] << 8;
  }

  while (cs > 0xFFFF) {
    cs = (cs & 0xFFFF) + (cs >> 16);
  }

  // In case the result is zero, send a ones' complement.
  if (cs == 0xFFFF) return cs;
  return ~cs;
}

auto udp_talker_bpf::pkt_details::build_pkt_hdr()
    -> const std::vector<std::uint8_t>& {
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

udp_talker_bpf::~udp_talker_bpf() {
  if (socket_fd_ != -1) close(socket_fd_);
  socket_fd_ = -1;
}

// For the "pimpl" pattern (using a unique_ptr on a forward declarated class),
// need to ensure that there is no definition (inline) of the constructor in the
// header file, else a move of "udp_talker_bpf" will not compile.
udp_talker_bpf::udp_talker_bpf() = default;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto udp_talker_bpf::init_packets(const struct sockaddr_in& source,
    const struct sockaddr_in& dest, std::uint16_t pkt_size) noexcept -> bool {
  if (socket_fd_ != -1) return false;

  // Padding. Minimum size of an Ethernet packet is 60 bytes. With a pkt_size of
  // zero, there is 18 padding bytes at the end.

  if (!ubench::net::is_multicast(dest.sin_addr)) {
    std::cerr << "BPF mode requires multicast destination IPv4" << std::endl;
    return false;
  }

  pkt_ = std::make_unique<pkt_details>();
  if (!ubench::net::get_ether_multicast(dest.sin_addr, &pkt_->dst_mac())) {
    std::cerr << "BPF mode requires multicast destination MAC" << std::endl;
    return false;
  }

  // Find the source MAC, by iterating over all available interfaces.
  constexpr auto flags = ubench::net::if_flags::MULTICAST |
                         ubench::net::if_flags::UP |
                         ubench::net::if_flags::RUNNING;
  auto interfaces = ubench::net::query_net_interfaces();
  std::string bpf_intf{};
  for (const auto& [name, intf] : interfaces) {
    if (intf.hw_addr() && (intf.status() & flags)) {
      for (const auto& ip : intf.inet()) {
        if (source.sin_addr.s_addr == ip.addr().s_addr) {
          if (intf.mtu() && *intf.mtu() < pkt_size + 28u) {
            std::cerr << "BPF mode size " << pkt_size << " <= UDP Max "
                      << (*intf.mtu() - 28)
                      << " would require IPv4 fragmentation" << std::endl;
            return false;
          }
          bpf_intf = name;

          // Under BPF, if we use a VLAN interface, it will automatically add
          // the VLAN tag for us. If we try and use the parent interface, the
          // MTU will be reduced by 4 bytes.

          // if (intf.vlan()) {
          //   pkt_->vlan_id() = intf.vlan()->id;
          //   bpf_intf = intf.vlan()->parent;
          // }

          pkt_->src_mac() = *intf.hw_addr();
          break;
        }
      }
    }
  }
  if (bpf_intf.empty()) {
    std::cerr << "BPF mode requires locally assigned IPv4 address for MAC"
              << std::endl;
    return false;
  }

  pkt_->src_ip() = source.sin_addr;
  pkt_->dst_ip() = dest.sin_addr;
  pkt_->src_port() = source.sin_port;
  pkt_->dst_port() = dest.sin_port;

  // Generate a packet
  pkt_->pkt_data().resize(pkt_size);
  std::uint8_t x = 0;
  for (auto& b : pkt_->pkt_data()) {
    b = x;
    x++;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  int fd = open("/dev/bpf", O_RDWR);
  if (fd < 0) {
    std::cerr << "BPF device interface cannot be opened" << std::endl;
    return false;
  }
  socket_fd_ = fd;

  ifreq ifr{};
  strlcpy(&ifr.ifr_name[0], bpf_intf.c_str(), IFNAMSIZ);
  if (ioctl(fd, BIOCSETIF, &ifr) == -1) {
    std::cerr << "BPF mode can't set the interface " << bpf_intf << std::endl;
    return false;
  }

  unsigned int hdr_complete = 1;
  if (ioctl(fd, BIOCSHDRCMPLT, &hdr_complete) == -1) {
    std::cerr << "BPF mode can't set the interface flag BIOCSHDRCOMPLT "
              << bpf_intf << std::endl;
    return false;
  }

  return true;
}

auto udp_talker_bpf::send_packets(std::uint16_t count) noexcept
    -> std::uint16_t {
  if (socket_fd_ == -1) return 0;

  // Note, we don't need to add a trailer of zeroes, as the network stack will
  // extend the size of the Ethernet packet with undefined data to meet the
  // minimum Ethernet specification size.
  std::array<iovec, 2> iov{};

  std::uint16_t sent = 0;
  for (std::uint16_t i = 0; i < count; i++) {
    // Reset the packet, that the new fragmentation identifier is calculated.
    pkt_->reset_pkt();

    iov[0].iov_base = const_cast<std::uint8_t *>(pkt_->build_pkt_hdr().data());
    iov[0].iov_len = pkt_->build_pkt_hdr().size();
    iov[1].iov_base = const_cast<std::uint8_t *>(pkt_->pkt_data().data());
    iov[1].iov_len = pkt_->pkt_data().size();

    auto result = writev(socket_fd_, iov.data(), iov.size());
    if (result > 0) {
      sent++;
    } else {
      if (errno != writev_errno_) {
        writev_errno_ = errno;
        perror("writev");
      }
    }
  }

  return sent;
}
