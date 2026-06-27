#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cerrno>
#include <limits>

#include "ubench/net.h"

namespace {

constexpr int MAX_TTL = std::numeric_limits<uint8_t>::max();

auto is_sockaddr_in4(const sockaddr_in& addr) {
  return addr.sin_family == AF_INET && addr.sin_addr.s_addr != INADDR_NONE &&
         addr.sin_addr.s_addr != INADDR_ANY;
}

}  // namespace

namespace ubench::net {

auto udp::get_sendbuf() noexcept -> stdext::expected<int, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }

  int buffsize = 0;
  socklen_t optlen = sizeof(buffsize);
  auto res = getsockopt(socket_, SOL_SOCKET, SO_SNDBUF, &buffsize, &optlen);
  if (res < 0) return stdext::unexpected{errno};
  return buffsize;
}

auto udp::set_sendbuf(int buf) noexcept -> stdext::expected<void, int> {
  if (!socket_ || buf <= 0) {
    return stdext::unexpected{EINVAL};
  }

  auto res =
      setsockopt(this->socket_, SOL_SOCKET, SO_SNDBUF, &buf, sizeof(buf));
  if (res) return stdext::unexpected{errno};
  return {};
}

auto udp::get_multicast_loop() noexcept -> stdext::expected<bool, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }

  char enabled = 0;
  socklen_t optlen = sizeof(enabled);
  auto res =
      getsockopt(socket_, IPPROTO_IP, IP_MULTICAST_LOOP, &enabled, &optlen);
  if (res) return stdext::unexpected{errno};
  return enabled != 0;
}

auto udp::set_multicast_loop(bool enable) noexcept
    -> stdext::expected<void, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }

  char loopch = enable ? 1 : 0;
  auto res = setsockopt(
      socket_, IPPROTO_IP, IP_MULTICAST_LOOP, &loopch, sizeof(loopch));
  if (res) return stdext::unexpected{errno};
  return {};
}

auto udp::multicast_join(const sockaddr_in& local) noexcept
    -> stdext::expected<void, int> {
  if (!socket_ || !is_sockaddr_in4(local)) {
    return stdext::unexpected{EINVAL};
  }

  auto res = setsockopt(
      socket_, IPPROTO_IP, IP_MULTICAST_IF, &(local.sin_addr), sizeof(in_addr));
  if (res) return stdext::unexpected{errno};
  return {};
}

auto udp::get_multicast_ttl() noexcept -> stdext::expected<int, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }

  unsigned char ttl = 0;
  socklen_t optlen = sizeof(ttl);
  auto res = getsockopt(socket_, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, &optlen);
  if (res) return stdext::unexpected{errno};
  return ttl;
}

auto udp::set_multicast_ttl(int ttl) noexcept -> stdext::expected<void, int> {
  if (!socket_ || ttl <= 0 || ttl > MAX_TTL) {
    return stdext::unexpected{EINVAL};
  }

  unsigned char mttl = ttl;
  auto res =
      setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_TTL, &mttl, sizeof(mttl));
  if (res) return stdext::unexpected{errno};
  return {};
}

auto udp::get_reuse_addr() noexcept -> stdext::expected<bool, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }

  int reuse = 0;
  socklen_t optlen = sizeof(reuse);
  auto res = getsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, &optlen);
  if (res) return stdext::unexpected{errno};
  return reuse != 0;
}

auto udp::set_reuse_addr(bool enable) noexcept -> stdext::expected<void, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }

  int reuse = enable ? 1 : 0;
  auto res =
      setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  if (res) return stdext::unexpected{errno};
  return {};
}

auto udp::get_reuse_port() noexcept -> stdext::expected<bool, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }

  int reuse = 0;
  socklen_t optlen = sizeof(reuse);
  auto res = getsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &reuse, &optlen);
  if (res) return stdext::unexpected{errno};
  return reuse != 0;
}

auto udp::set_reuse_port(bool enable) noexcept -> stdext::expected<void, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }

  int reuse = enable ? 1 : 0;
  auto res =
      setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
  if (res) return stdext::unexpected{errno};
  return {};
}

auto udp::ipv4hdr_length() noexcept -> stdext::expected<int, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }

  std::array<char, 40> options_buffer{};
  socklen_t options_len = options_buffer.size();
  auto res = getsockopt(
      socket_, IPPROTO_IP, IP_OPTIONS, options_buffer.data(), &options_len);
  if (res) return 20;
  return 20 + options_len;
}

auto udp::open() noexcept -> stdext::expected<void, int> {
  if (socket_) {
    return stdext::unexpected{EINVAL};
  }

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) return stdext::unexpected{errno};

  socket_ = fd;
  return {};
}

auto udp::open(const sockaddr_in& bind) noexcept
    -> stdext::expected<void, int> {
  if (!is_sockaddr_in4(bind)) {
    return stdext::unexpected{EINVAL};
  }

  auto res = open();
  if (!res) return res;

  // Systems programming.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto localaddr = reinterpret_cast<const sockaddr*>(&bind);
  auto bres = ::bind(socket_, localaddr, sizeof(bind));
  if (bres) return stdext::unexpected{errno};
  return {};
}

auto udp::send(const sockaddr_in& dest, std::string_view payload)
    -> stdext::expected<int, int> {
  if (!socket_ || !is_sockaddr_in4(dest)) {
    return stdext::unexpected{EINVAL};
  }

  // Systems programming.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto destaddr = reinterpret_cast<const sockaddr*>(&dest);
  ssize_t nbytes = sendto(
      socket_, payload.data(), payload.length(), 0, destaddr, sizeof(dest));

  if (nbytes < 0) return stdext::unexpected{errno};
  return nbytes;
}

}  // namespace ubench::net
