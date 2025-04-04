#ifndef POC_BPF_BPF_SOCKET_H
#define POC_BPF_BPF_SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <memory>

#include "ubench/file.h"
#include "udp_socket.h"

class bpf_socket : public udp_socket {
 public:
  bpf_socket() = delete;
  bpf_socket(const sockaddr_in& src, const sockaddr_in& dest);
  bpf_socket(const bpf_socket&) = delete;
  auto operator=(const bpf_socket&) -> bpf_socket& = delete;
  bpf_socket(bpf_socket&&) noexcept;
  auto operator=(bpf_socket&&) noexcept -> bpf_socket&;
  ;
  ~bpf_socket() override;

  operator bool() const noexcept override { return sock_; }
  auto send(const std::vector<std::byte>& data) noexcept
      -> stdext::expected<void, int> override;

 private:
  ubench::file::fdesc sock_{};
  unsigned int mtu_{};

  class l2_udp_packet;
  std::unique_ptr<l2_udp_packet> pkt_;
};

#endif
