#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>

#include "udp_talker.h"

udp_talker_bsdipv4::~udp_talker_bsdipv4() {
  if (socket_fd_ != -1) close(socket_fd_);
  socket_fd_ = -1;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto udp_talker_bsdipv4::init_packets(const struct sockaddr_in &source,
                                      const struct sockaddr_in &dest,
                                      std::uint16_t pkt_size) noexcept -> bool {
  if (socket_fd_ != -1) return false;

  std::uint8_t x = 0;
  buffer_.resize(pkt_size);
  for (auto &b : buffer_) {
    b = x;
    x++;
  }

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) return false;
  socket_fd_ = fd;

  bool is_multicast = (ntohl(dest.sin_addr.s_addr) & 0xF0000000) == 0xE0000000;
  if (is_multicast) {
    char loopch = 1;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loopch,
                   sizeof(loopch))) {
      perror("setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, 1)");
      close(fd);
      socket_fd_ = -1;
      return false;
    }

    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &(source.sin_addr),
                   sizeof(in_addr))) {
      perror("setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, source)");
      close(fd);
      socket_fd_ = -1;
      return false;
    }

    unsigned char mttl = 1;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &mttl, sizeof(mttl))) {
      perror("setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, 1)");
      close(fd);
      socket_fd_ = -1;
      return false;
    }
  }

  int reuseaddr = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr))) {
    perror("setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, 1)");
    close(fd);
    socket_fd_ = -1;
    return false;
  }

  int reuseport = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport))) {
    perror("setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, 1)");
    close(fd);
    socket_fd_ = -1;
    return false;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  if (bind(fd, reinterpret_cast<const sockaddr *>(&source),
           sizeof(sockaddr_in))) {
    perror("bind(fd, source)");
    close(fd);
    socket_fd_ = -1;
    return false;
  }

  // Remember the settings
  source_ = source;
  dest_ = dest;

  return true;
}

auto udp_talker_bsdipv4::send_packets(std::uint16_t count) noexcept
    -> std::uint16_t {
  if (socket_fd_ == -1) return 0;

  std::uint16_t sent = 0;
  for (std::uint16_t i = 0; i < count; i++) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto dest = reinterpret_cast<const sockaddr *>(&dest_);
    ssize_t nbytes = sendto(socket_fd_, buffer_.data(), buffer_.size(), 0, dest,
                            sizeof(dest_));
    if (nbytes < 0) return sent;
    sent++;
  }
  return sent;
}
