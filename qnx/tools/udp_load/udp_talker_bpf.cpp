#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <net/bpf.h>
#include <net/if.h>
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "ubench/net.h"
#include "ubench/string.h"
#include "udp_talker.h"

using namespace std::chrono_literals;

// For the "pimpl" pattern (using a unique_ptr on a forward declarated class),
// need to ensure that there is no definition (inline) of the constructor in the
// header file, else a move of "udp_talker_bpf" will not compile.
udp_talker_bpf::~udp_talker_bpf() = default;
udp_talker_bpf::udp_talker_bpf() = default;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto udp_talker_bpf::init_packets(const struct sockaddr_in& source,
    const struct sockaddr_in& dest, std::uint16_t pkt_size) noexcept -> bool {
  if (socket_fd_) return false;

  // Padding. Minimum size of an Ethernet packet is 60 bytes. With a pkt_size of
  // zero, there is 18 padding bytes at the end.

  if (!ubench::net::is_multicast(dest.sin_addr)) {
    std::cerr << "BPF mode requires multicast destination IPv4" << std::endl;
    return false;
  }

  pkt_ = std::make_unique<l2_eth_udp_pkt>();
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
          // Accessing const is fine. Clang 18.1.3 false warning.
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          if (intf.mtu() && *intf.mtu() < pkt_size + 28u) {
            std::cerr << "BPF mode size " << pkt_size
                      << " <= UDP Max "
                      // Clang 18.1.3 false warning.
                      // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
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

          // We know that `hw_addr` is defined in a prevous check. The value is
          // `const` so cannot change.
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
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
  ubench::file::fdesc fd = open("/dev/bpf", O_RDWR);
  if (!fd) {
    ubench::string::perror("BPF device interface cannot be opened");
    return false;
  }

  ifreq ifr{};
  strlcpy(&ifr.ifr_name[0], bpf_intf.c_str(), IFNAMSIZ);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (ioctl(fd, BIOCSETIF, &ifr) == -1) {
    ubench::string::perror("BPF mode can't set the interface " + bpf_intf);
    return false;
  }

  unsigned int hdr_complete = 1;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (ioctl(fd, BIOCSHDRCMPLT, &hdr_complete) == -1) {
    ubench::string::perror(
        "BPF mode can't set the interface flag BIOCSHDRCMPLT " + bpf_intf);
    return false;
  }

  socket_fd_ = std::move(fd);
  return true;
}

auto udp_talker_bpf::send_packets(std::uint16_t count) noexcept
    -> std::uint16_t {
  if (!socket_fd_) return 0;

  // Note, we don't need to add a trailer of zeroes, as the network stack will
  // extend the size of the Ethernet packet with undefined data to meet the
  // minimum Ethernet specification size.
  std::array<iovec, 2> iov{};

  std::uint16_t sent = 0;
  for (std::uint16_t i = 0; i < count; i++) {
    // Reset the packet, that the new fragmentation identifier is calculated.
    pkt_->reset_pkt();

    // Need to remove the const for the OS, even though the OS doesn't change
    // the values.
    //
    // NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast)
    iov[0].iov_base = const_cast<std::uint8_t*>(pkt_->build_pkt_hdr().data());
    iov[0].iov_len = pkt_->build_pkt_hdr().size();
    iov[1].iov_base = const_cast<std::uint8_t*>(pkt_->pkt_data().data());
    iov[1].iov_len = pkt_->pkt_data().size();
    // NOLINTEND(cppcoreguidelines-pro-type-const-cast)

    bool retry{};
    do {
      retry = false;
      auto result = writev(socket_fd_, iov.data(), iov.size());
      if (result < 0) {
        if (errno == ENOBUFS) {
          retry = true;
          if (delay(750us)) continue;
        }
        ubench::string::perror("writev()");
        return sent;
      }
    } while (retry);
    sent++;
  }

  return sent;
}
