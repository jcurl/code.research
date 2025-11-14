#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <net/bpf.h>
#include <net/if.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "l2_eth_udp_pkt.h"
#include "ubench/net.h"
#include "ubench/string.h"
#include "udp_talker.h"

using namespace std::chrono_literals;

// For the "pimpl" pattern (using a unique_ptr on a forward declarated class),
// need to ensure that there is no definition (inline) of the constructor in the
// header file, else a move of "udp_talker_bpfmm" will not compile.
udp_talker_bpfmm::~udp_talker_bpfmm() = default;
udp_talker_bpfmm::udp_talker_bpfmm() = default;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto udp_talker_bpfmm::init_packets(const struct sockaddr_in& source,
    const struct sockaddr_in& dest, std::uint16_t pkt_size) noexcept -> bool {
  if (socket_fd_) return false;

  // Padding. Minimum size of an Ethernet packet is 60 bytes. With a pkt_size of
  // zero, there is 18 padding bytes at the end.

  if (!ubench::net::is_multicast(dest.sin_addr)) {
    std::cerr << "BPF mode requires multicast destination IPv4" << std::endl;
    return false;
  }

  // Generate the sample packet
  std::unique_ptr<l2_eth_udp_pkt> pkt = std::make_unique<l2_eth_udp_pkt>();
  if (!ubench::net::get_ether_multicast(dest.sin_addr, &pkt->dst_mac())) {
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

          pkt->src_mac() = *intf.hw_addr();
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

  pkt->src_ip() = source.sin_addr;
  pkt->dst_ip() = dest.sin_addr;
  pkt->src_port() = source.sin_port;
  pkt->dst_port() = dest.sin_port;

  // Generate a packet
  pkt->pkt_data().resize(pkt_size);
  std::uint8_t x = 0;
  for (auto& b : pkt->pkt_data()) {
    b = x;
    x++;
  }

  // After the sample packet is created, we create multiple copies.
  //eth_packets_.resize(UIO_MAXIOV >> 1);
  eth_packets_.reserve(UIO_MAXIOV >> 1);
  msgvec_.resize(UIO_MAXIOV);

  for (size_t i = 0; i < UIO_MAXIOV; i += 2) {
    auto p = eth_packets_.emplace_back(*pkt);
    msgvec_[i].iov_base = nullptr;
    msgvec_[i].iov_len = 0;
    msgvec_[i + 1].iov_base = const_cast<std::uint8_t*>(p.pkt_data().data());
    msgvec_[i + 1].iov_len = pkt_size;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  ubench::file::fdesc fd = open("/dev/bpf", O_RDWR);
  if (!fd) {
    ubench::string::perror("BPF device interface cannot be opened");
    return false;
  }

  unsigned int mmwrite = 1;
  if (ioctl(fd, BIOCSMMWRITE, &mmwrite) == -1) {
    ubench::string::perror("BPF Multiwrite can't be set");
    return false;
  }

  ifreq ifr{};
  strlcpy(&ifr.ifr_name[0], bpf_intf.c_str(), IFNAMSIZ);
  if (ioctl(fd, BIOCSETIF, &ifr) == -1) {
    ubench::string::perror("BPF mode can't set the interface " + bpf_intf);
    return false;
  }

  unsigned int hdr_complete = 1;
  if (ioctl(fd, BIOCSHDRCMPLT, &hdr_complete) == -1) {
    ubench::string::perror(
        "BPF mode can't set the interface flag BIOCSHDRCOMPLT " + bpf_intf);
    return false;
  }

  socket_fd_ = std::move(fd);
  return true;
}

auto udp_talker_bpfmm::send_packets(std::uint16_t count) noexcept
    -> std::uint16_t {
  if (!socket_fd_) return 0;

  // TODO: Write the iovec

  // Note, we don't need to add a trailer of zeroes, as the network stack will
  // extend the size of the Ethernet packet with undefined data to meet the
  // minimum Ethernet specification size.
  // std::array<iovec, 2> iov{};

  // std::uint16_t sent = 0;
  // for (std::uint16_t i = 0; i < count; i++) {
  //   // Reset the packet, that the new fragmentation identifier is calculated.
  //   pkt_->reset_pkt();

  //   iov[0].iov_base = const_cast<std::uint8_t*>(pkt_->build_pkt_hdr().data());
  //   iov[0].iov_len = pkt_->build_pkt_hdr().size();
  //   iov[1].iov_base = const_cast<std::uint8_t*>(pkt_->pkt_data().data());
  //   iov[1].iov_len = pkt_->pkt_data().size();

  //   int result{};
  //   do {
  //     result = writev(socket_fd_, iov.data(), iov.size());
  //     if (result == -1) {
  //       if (errno == ENOBUFS) {
  //         if (delay(750us)) continue;
  //       }
  //       ubench::string::perror("writev()");
  //       return sent;
  //     }
  //   } while (result < 0);
  //   sent++;
  // }

  // return sent;
  return 0;
}
