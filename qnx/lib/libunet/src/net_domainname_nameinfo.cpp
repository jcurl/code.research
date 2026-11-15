#include <sys/socket.h>
#include <netdb.h>

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include "ubench/net.h"

namespace ubench::net {

namespace {

auto host(std::string_view hostname) -> std::string_view {
  return hostname.substr(0, hostname.find('.'));
}

// Returns the domain name of this host, derived from the reverse-resolved name
// of its address. Expected results, given hostname H and the getnameinfo name
// N:
//   - H = "rotorua",          N = "rotorua"           -> "" (no domain)
//   - H = "rotorua",          N = "rotorua.home.lan"  -> "home.lan"
// Returns nullopt if resolution fails or N's first label does not match H's.
auto domain(std::string_view hostname, std::string_view fqdn)
    -> std::optional<std::string_view> {
  auto dot = fqdn.find('.');
  if (dot == std::string_view::npos) {
    if (fqdn == hostname) return std::string_view{};
    return std::nullopt;
  }

  if (fqdn.substr(0, dot) == hostname) {
    return fqdn.substr(dot + 1);
  }
  return std::nullopt;
}

}  // namespace

[[nodiscard]] auto getdomainname() -> std::optional<std::string> {
  auto hostname = gethostname();
  if (!hostname) return std::nullopt;

  auto shortname = host(*hostname);

  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_CANONNAME;

  addrinfo* info = nullptr;
  int gai_result = ::getaddrinfo(hostname->data(), nullptr, &hints, &info);
  if (gai_result != 0 || info == nullptr) {
    if (info != nullptr) ::freeaddrinfo(info);
    return std::nullopt;
  }

  if (info->ai_canonname) {
    auto domainname = domain(shortname, std::string_view{info->ai_canonname});
    if (domainname && !domainname->empty()) return std::string{*domainname};
  }

  // Probably doesn't help at this point in time, but we try it anyway.
  std::array<char, NI_MAXHOST> resolved{};
  for (addrinfo* cursor = info; cursor != nullptr; cursor = cursor->ai_next) {
    int ni_result = ::getnameinfo(cursor->ai_addr, cursor->ai_addrlen,
        resolved.data(), resolved.size(), nullptr, 0, NI_NAMEREQD);
    if (ni_result != 0) continue;

    std::string_view fqdn{resolved.data()};
    auto domainname = domain(shortname, fqdn);
    if (!domainname || domainname->empty()) continue;

    ::freeaddrinfo(info);
    return std::string{*domainname};
  }

  ::freeaddrinfo(info);

  auto selfdomain = domain(shortname, *hostname);
  if (!selfdomain) return std::string{};
  return std::string{*selfdomain};
}

}  // namespace ubench::net
