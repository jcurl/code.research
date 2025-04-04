#include "config.h"

#include "bsd_socket.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cerrno>
#include <iostream>
#include <limits>

#include "stdext/expected.h"
#include "ubench/string.h"

namespace {

constexpr int MAX_TTL = std::numeric_limits<std::uint8_t>::max();

auto is_valid(const sockaddr_in& addr) noexcept -> bool {
  return !(addr.sin_family != AF_INET || addr.sin_addr.s_addr == INADDR_NONE);
}

auto multicast_loop(int fd, const sockaddr_in& group, bool enabled) noexcept
    -> stdext::expected<void, int> {
  if (fd < 0 || !is_valid(group)) return stdext::unexpected{EINVAL};

  char loopch = enabled ? 1 : 0;
  if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loopch, sizeof(loopch)) ==
      -1)
    return stdext::unexpected{errno};
  return {};
}

auto multicast_join(int fd, const sockaddr_in& addr) noexcept
    -> stdext::expected<void, int> {
  if (fd < 0 || !is_valid(addr)) return stdext::unexpected{EINVAL};

  sockaddr_in localaddr = addr;
  if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &(localaddr.sin_addr),
          sizeof(in_addr)) == -1)
    return stdext::unexpected{errno};
  return {};
}

auto multicast_ttl(int fd, int ttl) noexcept -> stdext::expected<void, int> {
  if (fd < 0 || ttl <= 0 || ttl > MAX_TTL) return stdext::unexpected{EINVAL};

  unsigned char mttl = ttl;
  if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &mttl, sizeof(mttl)) == -1)
    return stdext::unexpected{errno};
  return {};
}

auto reuseaddr(int fd, bool reuse) noexcept -> stdext::expected<void, int> {
  if (fd < 0) return stdext::unexpected{EINVAL};

  int value = reuse ? 1 : 0;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) == -1)
    return stdext::unexpected{errno};
  return {};
}

auto reuseport(int fd, [[maybe_unused]] bool reuse) noexcept
    -> stdext::expected<void, int> {
  if (fd < 0) return stdext::unexpected{EINVAL};

#if HAVE_SO_REUSEPORT
  int value = reuse ? 1 : 0;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value)) == -1)
    return stdext::unexpected{errno};
#endif
  return {};
}

auto bind(int fd, const sockaddr_in& addr) noexcept
    -> stdext::expected<void, int> {
  if (fd < 0 || !is_valid(addr)) return stdext::unexpected{EINVAL};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto localaddr = reinterpret_cast<const ::sockaddr*>(&addr);
  if (bind(fd, localaddr, sizeof(sockaddr_in)) == -1)
    return stdext::unexpected{errno};
  return {};
}

}  // namespace

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
bsd_socket::bsd_socket(const sockaddr_in& src, const sockaddr_in& dest)
    : dest_{dest} {
  ubench::file::fdesc fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (!fd) {
    std::cout << "socket(AF_INET, SOCK_DGRAM, 0) - "
              << ubench::string::perror(errno) << std::endl;
    return;
  }

  if (auto r = multicast_loop(fd, dest, true); !r) {
    std::cout << "setsockopt(IPPROTO_IP, IP_MULTICAST_LOOP, true) - "
              << ubench::string::perror(r.error()) << std::endl;
    return;
  }
  if (auto r = multicast_join(fd, src); !r) {
    std::cout << "setsockopt(IPPROTO_IP, IP_MULTICAST_IF, src) - "
              << ubench::string::perror(r.error()) << std::endl;
    return;
  }
  if (auto r = multicast_ttl(fd, 1); !r) {
    std::cout << "setsockopt(IPPROTO_IP, IP_MULTICAST_TTL, 1) - "
              << ubench::string::perror(r.error()) << std::endl;
    return;
  }
  if (auto r = reuseaddr(fd, true); !r) {
    std::cout << "setsockopt(SOL_SOCKET, SO_REUSEADDR, true) - "
              << ubench::string::perror(r.error()) << std::endl;
    return;
  }
  if (auto r = reuseport(fd, true); !r) {
    std::cout << "setsockopt(SOL_SOCKET, SO_REUSEPORT, true) - "
              << ubench::string::perror(r.error()) << std::endl;
    return;
  }
  if (auto r = bind(fd, src); !r) {
    std::cout << "bind(src) - " << ubench::string::perror(r.error())
              << std::endl;
    return;
  }

  // If we got this far, store the pointer, else it will be automatically closed
  // again.
  sock_ = std::move(fd);
}

auto bsd_socket::send(const std::vector<std::byte>& data) noexcept
    -> stdext::expected<void, int> {
  if (!sock_) return stdext::unexpected{EINVAL};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto destaddr = reinterpret_cast<const sockaddr*>(&dest_);
  ssize_t nbytes = sendto(
      sock_, data.data(), data.size(), 0, destaddr, sizeof(::sockaddr_in));
  if (nbytes == -1) return stdext::unexpected{errno};
  return {};
}
