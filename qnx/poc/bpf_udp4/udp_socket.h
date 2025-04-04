#ifndef POC_BPF_UDP_SOCKET_H
#define POC_BPF_UDP_SOCKET_H

#include <cstddef>
#include <vector>

#include "stdext/expected.h"

enum class send_mode {
  mode_sendto,  //< Use sendto() for sending packets.
  mode_bpf,     //< use BPF for sending packets.
};

class udp_socket {
 public:
  udp_socket() = default;
  udp_socket(const udp_socket&) = delete;
  auto operator=(const udp_socket&) -> udp_socket& = delete;
  udp_socket(udp_socket&&) = default;
  auto operator=(udp_socket&&) -> udp_socket& = default;
  virtual ~udp_socket() = default;

  virtual operator bool() const noexcept = 0;
  virtual auto send(const std::vector<std::byte>& data) noexcept
      -> stdext::expected<void, int> = 0;
};

#endif
