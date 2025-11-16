#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
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
// header file, else a move of "udp_talker_bpfmm" will not compile.
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
          if (intf.mtu() && *intf.mtu() < pkt_size + 28u) {
            std::cerr << "BPF mode size " << pkt_size << " <= UDP Max "
                      << (*intf.mtu() - 28)
                      << " would require IPv4 fragmentation" << std::endl;
            return false;
          }
          bpf_intf = name;

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

  // Prepare the packet pool
  bool allocated = eth_packets_.size() != 0;

  std::size_t maxsend = UIO_MAXIOV / 3;
  if (!allocated) {
    // Because of the malloc(), if we resize we'll lose all the pointers and
    // can't free it later.
    eth_packets_.resize(maxsend);
    msgvec_.resize(maxsend * 3);
  }

  for (std::size_t i = 0, j = 0; i < maxsend; i++, j += 3) {
    eth_packets_[i] = *pkt_;
    eth_packets_[i].reset_pkt();

    auto pkt_hdr = eth_packets_[i].build_pkt_hdr();
    auto pkt_dat = eth_packets_[i].pkt_data();

    // We have to allocate memory using "malloc", because we don't know how many
    // bytes to allocate upfront. QNX has a weird padding that is needed
    // depending on the size of the Ethernet packet. This memory should be freed
    // by the destructor.
    int s = pkt_hdr.size() + pkt_dat.size();
    int padding = BPF_WORDALIGN(s) - s;
    struct bpf_whdr* h =
        (struct bpf_whdr*)malloc(sizeof(struct bpf_whdr) + padding);
    h->bh_hdrlen = sizeof(struct bpf_whdr) + padding;
    h->bh_datalen = s;

    if (allocated) free(msgvec_[j].iov_base);
    msgvec_[j].iov_base = h;
    msgvec_[j].iov_len = h->bh_hdrlen;
    msgvec_[j + 1].iov_base = const_cast<std::uint8_t*>(pkt_hdr.data());
    msgvec_[j + 1].iov_len = pkt_hdr.size();
    msgvec_[j + 2].iov_base = const_cast<std::uint8_t*>(pkt_dat.data());
    msgvec_[j + 2].iov_len = pkt_dat.size();
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
        "BPF mode can't set the interface flag BIOCSHDRCMPLT " + bpf_intf);
    return false;
  }

  socket_fd_ = std::move(fd);
  return true;
}

auto udp_talker_bpfmm::send_packets(std::uint16_t count) noexcept
    -> std::uint16_t {
  if (!socket_fd_) return 0;

  std::uint16_t sent = 0;
  while (sent < count) {
    auto remain = std::min<std::uint16_t>(count - sent, eth_packets_.size());
    int result{};

    do {
      result = writev(socket_fd_, msgvec_.data(), remain * 3);
      if (result == -1) {
        if (errno == ENOBUFS) {
          if (delay(750us)) continue;
        }
        ubench::string::perror("writev()");
        return sent;
      }
    } while (result < 0);
    sent += remain;
  }
  return sent;
}

udp_talker_bpfmm::~udp_talker_bpfmm() {
  bool allocated = msgvec_.size() != 0;
  if (allocated) {
    // We don't have a class managing the variable sizesd bpf header object,
    // which is every third packet in the pool.
    for (std::size_t i = 0; i < msgvec_.size(); i += 3) {
      free(msgvec_[i].iov_base);
    }
  }
}
