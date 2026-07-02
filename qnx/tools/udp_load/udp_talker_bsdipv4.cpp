#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <ctime>
#include <iostream>

#include "ubench/net.h"
#include "ubench/string.h"
#include "udp_talker.h"

using namespace std::chrono_literals;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto udp_talker_bsd::init_packets(const struct sockaddr_in &source,
    const struct sockaddr_in &dest,
    [[maybe_unused]] std::uint16_t pkt_size) noexcept -> bool {
  if (udp_) return false;

  ubench::net::udp sock{};
  sock.set_reuse_addr(true);
  sock.set_reuse_port(true);
  auto ores = sock.open(source);
  if (!ores) {
    std::cerr << "sock.open() " << ubench::string::perror(ores.error())
              << std::endl;
    return false;
  }

  if (ubench::net::is_multicast(dest.sin_addr)) {
    auto mlres = sock.set_multicast_loopback(true);
    if (!mlres) {
      std::cerr << "sock.set_multicast_loopback(true) "
                << ubench::string::perror(mlres.error()) << std::endl;
      return false;
    }

    auto mires = sock.multicast_register_interface(source);
    if (!mires) {
      std::cerr << "sock.multicast_register_interface(source) "
                << ubench::string::perror(mires.error()) << std::endl;
      return false;
    }

    auto mtres = sock.set_multicast_ttl(1);
    if (!mtres) {
      std::cerr << "sock.set_multicast_ttl(1) "
                << ubench::string::perror(mtres.error()) << std::endl;
      return false;
    }
  }

  // Remember the settings
  source_ = source;
  dest_ = dest;
  udp_ = std::move(sock);
  return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto udp_talker_sendto::init_packets(const struct sockaddr_in &source,
    const struct sockaddr_in &dest, std::uint16_t pkt_size) noexcept -> bool {
  std::uint8_t x = 0;
  buffer_.resize(pkt_size);
  for (auto &b : buffer_) {
    b = static_cast<std::byte>(x);
    x++;
  }

  return udp_talker_bsd::init_packets(source, dest, pkt_size);
}

auto udp_talker_sendto::send_packets(std::uint16_t count) noexcept
    -> std::uint16_t {
  if (!udp_) return 0;

  std::uint16_t sent = 0;
  for (std::uint16_t i = 0; i < count; i++) {
    bool retry{};
    do {
      retry = false;
      auto sres = udp_.send(dest_, buffer_);
      if (!sres) {
        if (sres.error() == ENOBUFS) {
          retry = true;
          if (delay(750us)) continue;
        }
        std::cerr << "udp_.sendto() " << ubench::string::perror(sres.error());
        return sent;
      }
    } while (retry);
    sent++;
  }
  return sent;
}

#if HAVE_SENDMMSG
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto udp_talker_sendmmsg::init_packets(const struct sockaddr_in &source,
    const struct sockaddr_in &dest, std::uint16_t pkt_size) noexcept -> bool {
  // Prepare up to 1024 Ethernet packets (UIO_MAXIOV) which is supported on QNX
  // and Linux.
  eth_packets_.resize(UIO_MAXIOV);
  msgvec_.resize(UIO_MAXIOV);
  msgpool_.resize(UIO_MAXIOV);
  for (size_t i = 0; i < UIO_MAXIOV; i++) {
    std::vector<std::uint8_t> packet(pkt_size);
    std::uint8_t x = i;
    for (auto &b : packet) {
      b = x;
      x++;
    }

    msgvec_[i].iov_base = packet.data();
    msgvec_[i].iov_len = pkt_size;

    msgpool_[i].msg_hdr.msg_name = static_cast<void *>(&dest_);
    msgpool_[i].msg_hdr.msg_namelen = sizeof(dest_);
    msgpool_[i].msg_hdr.msg_iov = &msgvec_[i];
    msgpool_[i].msg_hdr.msg_iovlen = 1;
    msgpool_[i].msg_hdr.msg_control = nullptr;
    msgpool_[i].msg_hdr.msg_controllen = 0;
    msgpool_[i].msg_hdr.msg_flags = 0;

    eth_packets_[i] = std::move(packet);
  }

  return udp_talker_bsd::init_packets(source, dest, pkt_size);
}

auto udp_talker_sendmmsg::send_packets(std::uint16_t count) noexcept
    -> std::uint16_t {
  if (!udp_) return 0;

  std::uint16_t sent = 0;
  while (sent < count) {
    auto remain = std::min<std::uint16_t>(count - sent, UIO_MAXIOV);
    bool retry{};
    do {
      retry = false;
      auto result = sendmmsg(udp_, msgpool_.data(), remain, 0);
      if (result < 0) {
        if (errno == ENOBUFS) {
          retry = true;
          if (delay(750us)) continue;
        }
        ubench::string::perror("sendmmsg()");
        return sent;
      }
      sent += result;
    } while (retry);
  }
  return sent;
}
#endif
