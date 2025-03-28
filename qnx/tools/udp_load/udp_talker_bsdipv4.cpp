#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <ctime>

#include "ubench/net.h"
#include "ubench/string.h"
#include "udp_talker.h"

using namespace std::chrono_literals;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto udp_talker_bsd::init_packets(const struct sockaddr_in &source,
    const struct sockaddr_in &dest,
    [[maybe_unused]] std::uint16_t pkt_size) noexcept -> bool {
  if (socket_fd_) return false;

  ubench::file::fdesc fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (!fd) return false;

  if (ubench::net::is_multicast(dest.sin_addr)) {
    char loopch = 1;
    if (setsockopt(
            fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loopch, sizeof(loopch))) {
      ubench::string::perror(
          "setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, 1)");
      return false;
    }

    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &(source.sin_addr),
            sizeof(in_addr))) {
      ubench::string::perror(
          "setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, source)");
      return false;
    }

    unsigned char mttl = 1;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &mttl, sizeof(mttl))) {
      ubench::string::perror("setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, 1)");
      return false;
    }
  }

  int reuseaddr = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr))) {
    ubench::string::perror("setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, 1)");
    return false;
  }

#if HAVE_SO_REUSEPORT
  int reuseport = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport))) {
    ubench::string::perror("setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, 1)");
    return false;
  }
#endif

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  if (bind(fd, reinterpret_cast<const sockaddr *>(&source),
          sizeof(sockaddr_in))) {
    ubench::string::perror("bind(fd, source)");
    return false;
  }

  // Remember the settings
  source_ = source;
  dest_ = dest;

  socket_fd_ = std::move(fd);

  return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto udp_talker_sendto::init_packets(const struct sockaddr_in &source,
    const struct sockaddr_in &dest, std::uint16_t pkt_size) noexcept -> bool {
  std::uint8_t x = 0;
  buffer_.resize(pkt_size);
  for (auto &b : buffer_) {
    b = x;
    x++;
  }

  return udp_talker_bsd::init_packets(source, dest, pkt_size);
}

auto udp_talker_sendto::send_packets(std::uint16_t count) noexcept
    -> std::uint16_t {
  if (!socket_fd_) return 0;

  std::uint16_t sent = 0;
  for (std::uint16_t i = 0; i < count; i++) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto dest = reinterpret_cast<const sockaddr *>(&dest_);
    ssize_t nbytes{};
    do {
      nbytes = sendto(
          socket_fd_, buffer_.data(), buffer_.size(), 0, dest, sizeof(dest_));
      if (nbytes < 0) {
        if (errno == ENOBUFS) {
          if (delay(750us)) continue;
        }
        ubench::string::perror("sendto()");
        return sent;
      }
    } while (nbytes < 0);
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
  eth_packets_.resize(1024);
  msgvec_.resize(1024);
  msgpool_.resize(1024);
  for (size_t i = 0; i < 1024; i++) {
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
  if (!socket_fd_) return 0;

  std::uint16_t sent = 0;
  while (sent < count) {
    auto remain = std::min(count - sent, 1024);
    int result{};
    do {
      result = sendmmsg(socket_fd_, msgpool_.data(), remain, 0);
      if (result == -1) {
        if (errno == ENOBUFS) {
          if (delay(750us)) continue;
        }
        ubench::string::perror("sendmmsg()");
        return sent;
      }
    } while (result < 0);
    sent += result;
  }
  return sent;
}
#endif
