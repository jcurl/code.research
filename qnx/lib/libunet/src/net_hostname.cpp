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

[[nodiscard]] auto gethostname() -> std::optional<std::string> {
  std::array<char, _POSIX_HOST_NAME_MAX> hostname{};

  int result = ::gethostname(hostname.data(), hostname.size());
  if (result) return std::nullopt;

  // Find the length, which is up to the first dot '.', NUL terminator, or the
  // maximum length.
  size_t len = 0;
  while (len < hostname.size()) {
    // Clearly within boundaries.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    if (hostname[len] == '.' || hostname[len] == 0) break;
    len++;
  }

  return std::string{hostname.data(), len};
}

[[nodiscard]] auto getdomainname() -> std::optional<std::string> {
  auto phostname = gethostname();
  if (!phostname) return std::nullopt;

  // Fixes false positive for "Inner pointer of container used after
  // re/deallocation [clang-analyzer-cplusplus.InnerPointer]" with GCC 11.4.0.
  std::string& hostname = *phostname;

  ::sethostent(0);
  struct hostent* h = nullptr;
  while ((h = ::gethostent()) != nullptr) {
    if ((h->h_name && std::strncmp(h->h_name, hostname.c_str(),
                          hostname.length()) == 0) &&
        // Operating system interop.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        h->h_name[hostname.length()] == '.') {
      ::endhostent();
      if (hostname.length() == std::string::npos) return std::nullopt;
      // Operating system interop.
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      return std::string{h->h_name + hostname.length() + 1};
    }
  }
  ::endhostent();
  return std::nullopt;
}

}  // namespace ubench::net
