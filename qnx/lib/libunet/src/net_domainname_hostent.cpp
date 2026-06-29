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
