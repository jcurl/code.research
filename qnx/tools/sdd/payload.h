#ifndef SDD_PAYLOAD_H
#define SDD_PAYLOAD_H

#include <string>

#include "ubench/net.h"

// Forward declarations for writing values into JSON. This must be in the public
// header file, so that `payload.cpp` sees the definition before including
// "json_writer.h" for its template expansion.
namespace std {
auto to_string(const ubench::net::if_ipv4& addr) -> std::string;
auto to_string(const ubench::net::ether_addr& addr) -> std::string;
}  // namespace std

/// @brief Class to maintain state for generating a UDP payload for discovery.
class payload {
 public:
  payload(const payload&) = delete;
  auto operator=(const payload&) -> payload& = delete;
  payload(payload&&) noexcept = default;
  auto operator=(payload&&) noexcept -> payload& = default;
  ~payload() = default;

  /// @brief Generate a payload string to be sent in a UDP packet for discovery.
  ///
  /// @return The generated payload string.
  auto generate() -> std::string;

 private:
};

#endif
