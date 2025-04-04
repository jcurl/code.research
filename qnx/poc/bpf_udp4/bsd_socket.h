#ifndef POC_BPF_BSD_SOCKET_H
#define POC_BPF_BSD_SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ubench/file.h"
#include "udp_socket.h"

class bsd_socket : public udp_socket {
 public:
  bsd_socket() = delete;
  bsd_socket(const sockaddr_in& src, const sockaddr_in& dest);
  bsd_socket(const bsd_socket&) = delete;
  auto operator=(const bsd_socket&) -> bsd_socket& = delete;
  bsd_socket(bsd_socket&&) = default;
  auto operator=(bsd_socket&&) -> bsd_socket& = default;
  ~bsd_socket() override = default;

  operator bool() const noexcept override { return sock_; }
  auto send(const std::vector<std::byte>& data) noexcept
      -> stdext::expected<void, int> override;

 private:
  ubench::file::fdesc sock_{};
  sockaddr_in dest_{};
};

#endif
