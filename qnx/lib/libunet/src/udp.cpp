#include "config.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cerrno>
#include <limits>

#include "ubench/net.h"

namespace internal {
namespace {

auto is_valid_sockaddr(const sockaddr_in& addr) {
  return addr.sin_family == AF_INET && addr.sin_addr.s_addr != INADDR_NONE &&
         addr.sin_addr.s_addr != INADDR_ANY;
}

auto is_valid_bind_addr(const sockaddr_in& addr) {
  return addr.sin_family == AF_INET && addr.sin_addr.s_addr != INADDR_NONE;
}

auto get_sendbuf(int fd) -> stdext::expected<int, int> {
  int buffsize = 0;
  socklen_t optlen = sizeof(buffsize);
  auto res = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffsize, &optlen);
  if (res < 0) return stdext::unexpected{errno};
  return buffsize;
}

auto set_sendbuf(int fd, int buf) -> stdext::expected<void, int> {
  if (buf <= 0) {
    return stdext::unexpected{EINVAL};
  }

  auto res = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buf, sizeof(buf));
  if (res < 0) return stdext::unexpected{errno};
  return {};
}

auto get_multicast_loopback(int fd) -> stdext::expected<bool, int> {
  char enabled = 0;
  socklen_t optlen = sizeof(enabled);
  auto res = getsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &enabled, &optlen);
  if (res) return stdext::unexpected{errno};
  return enabled != 0;
}

auto set_multicast_loopback(int fd, bool enable)
    -> stdext::expected<void, int> {
  char loopch = enable ? 1 : 0;
  auto res =
      setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loopch, sizeof(loopch));
  if (res) return stdext::unexpected{errno};
  return {};
}

auto multicast_register_interface(int fd, const in_addr local)
    -> stdext::expected<void, int> {
  if (local.s_addr == INADDR_NONE) {
    return stdext::unexpected{EINVAL};
  }

  auto res =
      setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &local, sizeof(in_addr));
  if (res) return stdext::unexpected{errno};
  return {};
}

auto get_multicast_ttl(int fd) -> stdext::expected<std::uint8_t, int> {
  std::uint8_t ttl = 0;
  socklen_t optlen = sizeof(ttl);
  auto res = getsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, &optlen);
  if (res) return stdext::unexpected{errno};
  return ttl;
}

auto set_multicast_ttl(int fd, std::uint8_t ttl)
    -> stdext::expected<void, int> {
  if (ttl <= 0) {
    return stdext::unexpected{EINVAL};
  }

  auto res = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
  if (res) return stdext::unexpected{errno};
  return {};
}

#if HAVE_SO_REUSEADDR
auto get_reuse_addr([[maybe_unused]] int fd) -> stdext::expected<bool, int> {
  int reuse = 0;
  socklen_t optlen = sizeof(reuse);
  auto res = getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, &optlen);
  if (res) return stdext::unexpected{errno};
  return reuse != 0;
}

auto set_reuse_addr([[maybe_unused]] int fd, [[maybe_unused]] bool enable)
    -> stdext::expected<void, int> {
  int reuse = enable ? 1 : 0;
  auto res = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  if (res) return stdext::unexpected{errno};
  return {};
}
#endif

#if HAVE_SO_REUSEPORT
auto get_reuse_port([[maybe_unused]] int fd) -> stdext::expected<bool, int> {
  int reuse = 0;
  socklen_t optlen = sizeof(reuse);
  auto res = getsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuse, &optlen);
  if (res) return stdext::unexpected{errno};
  return reuse != 0;
}

auto set_reuse_port([[maybe_unused]] int fd, [[maybe_unused]] bool enable)
    -> stdext::expected<void, int> {
  int reuse = enable ? 1 : 0;
  auto res = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
  if (res) return stdext::unexpected{errno};
  return {};
}
#endif

auto ipv4hdr_length(int fd) -> stdext::expected<int, int> {
  std::array<char, 40> options_buffer{};
  socklen_t options_len = options_buffer.size();
  auto res = getsockopt(
      fd, IPPROTO_IP, IP_OPTIONS, options_buffer.data(), &options_len);
  if (res) return 20;
  return 20 + options_len;
}

auto open() -> stdext::expected<ubench::file::fdesc, int> {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) return stdext::unexpected{errno};
  return fd;
}

auto bind(int fd, const sockaddr_in& bind) -> stdext::expected<void, int> {
  // Systems programming.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto localaddr = reinterpret_cast<const sockaddr*>(&bind);
  auto res = ::bind(fd, localaddr, sizeof(bind));
  if (res) return stdext::unexpected{errno};
  return {};
}

auto connect(int fd, const sockaddr_in& dest) -> stdext::expected<void, int> {
  // Systems programming.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto destaddr = reinterpret_cast<const sockaddr*>(&dest);
  auto res = ::connect(fd, destaddr, sizeof(dest));
  if (res) return stdext::unexpected{errno};
  return {};
}

}  // namespace
}  // namespace internal

namespace ubench::net {

auto udp::get_sendbuf() noexcept -> stdext::expected<int, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }
  return internal::get_sendbuf(socket_);
}

auto udp::set_sendbuf(int buf) noexcept -> stdext::expected<void, int> {
  if (!socket_ || buf <= 0) {
    return stdext::unexpected{EINVAL};
  }
  return internal::set_sendbuf(socket_, buf);
}

auto udp::get_multicast_loopback() noexcept -> stdext::expected<bool, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }
  return internal::get_multicast_loopback(socket_);
}

auto udp::set_multicast_loopback(bool enable) noexcept
    -> stdext::expected<void, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }
  return internal::set_multicast_loopback(socket_, enable);
}

auto udp::multicast_register_interface(const in_addr local) noexcept
    -> stdext::expected<void, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }
  return internal::multicast_register_interface(socket_, local);
}

auto udp::multicast_register_interface(const sockaddr_in& local) noexcept
    -> stdext::expected<void, int> {
  if (!socket_ || !internal::is_valid_bind_addr(local)) {
    return stdext::unexpected{EINVAL};
  }
  return internal::multicast_register_interface(socket_, local.sin_addr);
}

auto udp::get_multicast_ttl() noexcept -> stdext::expected<std::uint8_t, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }
  return internal::get_multicast_ttl(socket_);
}

auto udp::set_multicast_ttl(std::uint8_t ttl) noexcept
    -> stdext::expected<void, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }
  return internal::set_multicast_ttl(socket_, ttl);
}

auto udp::get_reuse_addr() noexcept -> stdext::expected<bool, int> {
#if HAVE_SO_REUSEADDR
  // Cygwin must have the flag set after open, and before bind. So we do this in
  // the open() call.
  if (socket_) {
    return internal::get_reuse_addr(socket_);
  }
  return reuse_addr_;
#else
  return stdext::unexpected{ENOTSUP};
#endif
}

auto udp::set_reuse_addr([[maybe_unused]] bool enable) noexcept
    -> stdext::expected<void, int> {
#if HAVE_SO_REUSEADDR
  // Cygwin must have the flag set after open, and before bind. So we do this in
  // the open() call.
  if (socket_) {
    return stdext::unexpected{EINVAL};
  }
  reuse_addr_ = enable;
  return {};
#else
  return stdext::unexpected{ENOTSUP};
#endif
}

auto udp::get_reuse_port() noexcept -> stdext::expected<bool, int> {
#if HAVE_SO_REUSEPORT
  // Cygwin must have the flag set after open, and before bind. So we do this in
  // the open() call.
  if (socket_) {
    return internal::get_reuse_port(socket_);
  }
  return reuse_port_;
#else
  return stdext::unexpected{ENOTSUP};
#endif
}

auto udp::set_reuse_port([[maybe_unused]] bool enable) noexcept
    -> stdext::expected<void, int> {
#if HAVE_SO_REUSEPORT
  // Cygwin must have the flag set after open, and before bind. So we do this in
  // the open() call.
  if (socket_) {
    return stdext::unexpected{EINVAL};
  }
  reuse_port_ = enable;
  return {};
#else
  return stdext::unexpected{ENOTSUP};
#endif
}

auto udp::ipv4hdr_length() noexcept -> stdext::expected<int, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }
  return internal::ipv4hdr_length(socket_);
}

auto udp::open(const sockaddr_in& bind) noexcept
    -> stdext::expected<void, int> {
  if (socket_) {
    return stdext::unexpected{EINVAL};
  }

  if (!internal::is_valid_bind_addr(bind)) {
    return stdext::unexpected{EINVAL};
  }

  auto ores = internal::open();
  if (!ores) return stdext::unexpected{ores.error()};
  ubench::file::fdesc fd = std::move(*ores);

#if HAVE_SO_REUSEADDR
  if (reuse_addr_) {
    auto rares = internal::set_reuse_addr(fd, reuse_addr_);
    if (!rares) return stdext::unexpected{rares.error()};
  }
#endif

#if HAVE_SO_REUSEPORT
  if (reuse_port_) {
    auto rpres = internal::set_reuse_port(fd, reuse_port_);
    if (!rpres) return stdext::unexpected{rpres.error()};
  }
#endif

  auto bres = internal::bind(fd, bind);
  if (!bres) return stdext::unexpected{bres.error()};

  socket_ = std::move(fd);
  return {};
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto udp::open(const sockaddr_in& bind, const sockaddr_in& dest) noexcept
    -> stdext::expected<void, int> {
  if (socket_) {
    return stdext::unexpected{EINVAL};
  }

  if (!internal::is_valid_bind_addr(bind)) {
    return stdext::unexpected{EINVAL};
  }

  if (!internal::is_valid_sockaddr(dest)) {
    return stdext::unexpected{EINVAL};
  }

  auto ores = internal::open();
  if (!ores) return stdext::unexpected{ores.error()};
  ubench::file::fdesc fd = std::move(*ores);

  auto bres = internal::bind(fd, bind);
  if (!bres) return stdext::unexpected{bres.error()};

  auto cres = internal::connect(fd, dest);
  if (!cres) return stdext::unexpected{cres.error()};

  // Some OSes will still send multicast without joining (e.g. Linux, but not
  // QNX). To make programming a little more robust, if the user sets the
  // destination to be a multicast address, we try to register the local
  // address.
  if (is_multicast(dest.sin_addr)) {
    auto mjres = internal::multicast_register_interface(fd, bind.sin_addr);
    if (!mjres) return stdext::unexpected{mjres.error()};
  }

  socket_ = std::move(fd);
  return {};
}

auto udp::send(std::string_view payload) -> stdext::expected<int, int> {
  if (!socket_) {
    return stdext::unexpected{EINVAL};
  }

  ssize_t nbytes = ::send(socket_, payload.data(), payload.length(), 0);
  if (nbytes < 0) return stdext::unexpected{errno};
  return nbytes;
}

auto udp::send(const sockaddr_in& dest, std::string_view payload)
    -> stdext::expected<int, int> {
  if (!socket_ || !internal::is_valid_sockaddr(dest)) {
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
