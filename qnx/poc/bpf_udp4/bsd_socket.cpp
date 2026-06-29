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

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
bsd_socket::bsd_socket(const sockaddr_in& src, const sockaddr_in& dest) {
  auto udp = ubench::net::udp{};
  if (auto rares = udp.set_reuse_addr(true); !rares) {
    std::cout << "udp.set_reuse_addr(true) - "
              << ubench::string::perror(rares.error()) << std::endl;
    return;
  }

  if (auto rpres = udp.set_reuse_port(true);
      !rpres && rpres.error() != ENOTSUP) {
    std::cout << "udp.set_reuse_port(true) - "
              << ubench::string::perror(rpres.error()) << std::endl;
    return;
  }

  if (auto ores = udp.open(src, dest); !ores) {
    std::cout << "udp.open(src, dest) - "
              << ubench::string::perror(ores.error()) << std::endl;
    return;
  }

  if (auto mtres = udp.set_multicast_ttl(1); !mtres) {
    std::cout << "udp.set_multicast_ttl(1) - "
              << ubench::string::perror(mtres.error()) << std::endl;
    return;
  }

  // If we got this far, store the pointer, else it will be automatically closed
  // again.
  udp_ = std::move(udp);
}

auto bsd_socket::send(const std::vector<std::byte>& data) noexcept
    -> stdext::expected<void, int> {
  if (!udp_) return stdext::unexpected{EINVAL};
  auto sres = udp_.send(data);
  if (!sres) return stdext::unexpected{sres.error()};
  return {};
}
