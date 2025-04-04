#include "config.h"

#include "bpf_socket.h"

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

#include <iostream>

#include "stdext/expected.h"
#include "ubench/net.h"
#include "ubench/string.h"
#include "bpf_l2_udp_packet.h"

namespace {

auto find_ipv4(const sockaddr_in& src)
    -> std::optional<ubench::net::interface> {
  constexpr auto flags = ubench::net::if_flags::MULTICAST |
                         ubench::net::if_flags::UP |
                         ubench::net::if_flags::RUNNING;
  auto ifs = ubench::net::query_net_interfaces();
  for (const auto& [ifn, ifd] : ifs) {
    if (ifd.hw_addr() && (ifd.status() & flags)) {
      for (const auto& ipv4addr : ifd.inet()) {
        if (ipv4addr.addr().s_addr == src.sin_addr.s_addr) {
          return ifd;
        }
      }
    }
  }
  return {};
}

auto bpf_setintf(int fd, const std::string& name)
    -> stdext::expected<void, int> {
  if (fd < 0) return stdext::unexpected{EINVAL};

  ifreq ifr{};
  strlcpy(&ifr.ifr_name[0], name.c_str(), IFNAMSIZ);
  if (ioctl(fd, BIOCSETIF, &ifr) == -1) return stdext::unexpected{errno};
  return {};
}

auto bpf_sethdr(int fd, bool complete) -> stdext::expected<void, int> {
  if (fd < 0) return stdext::unexpected{EINVAL};

  unsigned int hdr_complete = complete ? 1 : 0;
  if (ioctl(fd, BIOCSHDRCMPLT, &hdr_complete) == -1)
    return stdext::unexpected{errno};
  return {};
}

}  // namespace

bpf_socket::bpf_socket(bpf_socket&&) noexcept = default;
auto bpf_socket::operator=(bpf_socket&&) noexcept -> bpf_socket& = default;
bpf_socket::~bpf_socket() = default;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
bpf_socket::bpf_socket(const sockaddr_in& src, const sockaddr_in& dest)
    : pkt_{std::make_unique<bpf_socket::l2_udp_packet>()} {
  // We can't automatically generate ports. User must provide them explicitly.
  if (src.sin_port == 0 || dest.sin_port == 0) return;

  // Sending UDPv4 can only support multicast, as we don't implement ARP and
  // other protocols.
  if (!ubench::net::is_multicast(dest.sin_addr)) return;
  ubench::net::ether_addr destmac{};
  if (!ubench::net::get_ether_multicast(dest.sin_addr, &destmac)) return;
  pkt_->write_dest_mac(destmac);
  pkt_->write_dest_ip(dest);

  // Query all interfaces and find the source MAC and the MTU.
  auto srcintf = find_ipv4(src);
  if (!srcintf) return;
  mtu_ = *srcintf->mtu();
  pkt_->write_src_mac(*srcintf->hw_addr());
  pkt_->write_src_ip(src);

  // Initialise the BPF interface.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  ubench::file::fdesc fd = open("/dev/bpf", O_RDWR);
  if (!fd) {
    std::cout << "open('/dev/bpf', O_RDWR) - " << ubench::string::perror(errno)
              << std::endl;
    return;
  }

  if (auto r = bpf_setintf(fd, srcintf->name()); !r) {
    std::cout << "ioctl(BIOCSETIF, " << srcintf->name() << ") - "
              << ubench::string::perror(r.error()) << std::endl;
    return;
  }
  if (auto r = bpf_sethdr(fd, true); !r) {
    std::cout << "ioctl(BIOCSHDRCMPLT, true) - "
              << ubench::string::perror(r.error()) << std::endl;
    return;
  }

  // If we got this far, store the pointer, else it will be automatically closed
  // again.
  sock_ = std::move(fd);
}

auto bpf_socket::send(const std::vector<std::byte>& data) noexcept
    -> stdext::expected<void, int> {
  if (!sock_) return stdext::unexpected{EINVAL};

  std::array<iovec, 2> iov{};
  if (data.size() <= mtu_ - 28) {
    // IPv4 header is 20 bytes; UDP header is 8 bytes. Leaving remaining for
    // data. No fragmentation required in this path.
    auto hdr = pkt_->get_header(data);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    iov[0].iov_base = const_cast<std::byte*>(hdr.data());
    iov[0].iov_len = hdr.hdr_size();

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    iov[1].iov_base = const_cast<std::byte*>(data.data());
    iov[1].iov_len = hdr.payload_len();

    auto result = writev(sock_, iov.data(), iov.size());
    if (result == -1) {
      auto r = errno;
      std::cout << "ioctl(BIOCSHDRCMPLT, true) - " << ubench::string::perror(r)
                << std::endl;
      return stdext::unexpected{r};
    }
    return {};
  }

  // Fragmentation of the IPv4 packet needed.
  std::size_t offset{};
  while (offset < data.size()) {
    auto hdr = pkt_->get_header(data, offset, mtu_);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    iov[0].iov_base = const_cast<std::byte*>(hdr.data());
    iov[0].iov_len = hdr.hdr_size();

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    iov[1].iov_base = const_cast<std::byte*>(data.data()) + offset;
    iov[1].iov_len = hdr.payload_len();

    auto result = writev(sock_, iov.data(), iov.size());
    if (result == -1) {
      auto r = errno;
      std::cout << "ioctl(BIOCSHDRCMPLT, true) - " << ubench::string::perror(r)
                << std::endl;
      return stdext::unexpected{r};
    }

    offset += hdr.payload_len();
  }

  return {};
}
