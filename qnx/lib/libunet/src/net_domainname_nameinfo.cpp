#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <climits>
#include <cstring>

#include "ubench/net.h"
#include "ubench/string.h"

namespace ubench::net {

[[nodiscard]] auto getdomainname() -> std::optional<std::string> {
  auto hostname = gethostname();
  if (!hostname) return std::nullopt;

  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;

  addrinfo* info = nullptr;
  const int gai_result =
      ::getaddrinfo(hostname->data(), nullptr, &hints, &info);
  if (gai_result != 0 || info == nullptr) {
    if (info != nullptr) ::freeaddrinfo(info);
    return std::nullopt;
  }

  std::array<char, NI_MAXHOST> resolved{};
  for (addrinfo* cursor = info; cursor != nullptr; cursor = cursor->ai_next) {
    const int ni_result = ::getnameinfo(cursor->ai_addr, cursor->ai_addrlen,
        resolved.data(), resolved.size(), nullptr, 0, 0);
    if (ni_result == 0) {
      ::freeaddrinfo(info);

      size_t len = strlen(resolved.data());
      return std::string{resolved.data(), len};
    }
  }

  ::freeaddrinfo(info);
  return std::nullopt;
}

}  // namespace ubench::net
