#ifndef BENCHMARK_UDP_L2_ETH_UDP_PKT_H
#define BENCHMARK_UDP_L2_ETH_UDP_PKT_H

#include <ubench/net.h>

#include <cstdint>
#include <vector>

class l2_eth_udp_pkt {
 public:
  l2_eth_udp_pkt() = default;
  ~l2_eth_udp_pkt() = default;
  l2_eth_udp_pkt(const l2_eth_udp_pkt&) = default;
  auto operator=(const l2_eth_udp_pkt&) -> l2_eth_udp_pkt& = default;
  l2_eth_udp_pkt(l2_eth_udp_pkt&& other) = default;
  auto operator=(l2_eth_udp_pkt&&) -> l2_eth_udp_pkt& = default;

  [[nodiscard]] auto dst_mac() noexcept -> ubench::net::ether_addr& {
    return dst_mac_;
  }
  [[nodiscard]] auto src_mac() noexcept -> ubench::net::ether_addr& {
    return src_mac_;
  }
  [[nodiscard]] auto vlan_id() noexcept -> std::uint16_t& { return vlan_id_; }
  [[nodiscard]] auto fragmentation_id() noexcept -> std::uint16_t& {
    return fragmentation_id_;
  }
  [[nodiscard]] auto ttl() noexcept -> std::uint8_t& { return ttl_; }
  [[nodiscard]] auto dst_ip() noexcept -> in_addr& { return dst_ip_; }
  [[nodiscard]] auto src_ip() noexcept -> in_addr& { return src_ip_; }
  [[nodiscard]] auto dst_port() noexcept -> std::uint16_t& { return dst_port_; }
  [[nodiscard]] auto src_port() noexcept -> std::uint16_t& { return src_port_; }

  auto reset_pkt() noexcept -> void { reset_ = true; }

  [[nodiscard]] auto pkt_data() noexcept -> std::vector<std::uint8_t>& {
    return pkt_data_;
  }

  /// @brief Build the packet header.
  ///
  /// Call after all fields are assigned. It will rebuild the packet the first
  /// time. If you modify any of the fields after rebuilding, call reset_pkt()
  /// so that this function will rebuild the packet header again (else it just
  /// returns the last calculated value).
  ///
  /// @return the packet header.
  [[nodiscard]] auto build_pkt_hdr() -> const std::vector<std::uint8_t>&;

 private:
  bool reset_{true};
  ubench::net::ether_addr dst_mac_{};
  ubench::net::ether_addr src_mac_{};
  std::uint16_t vlan_id_{65535};
  std::uint16_t fragmentation_id_{0};
  std::uint8_t ttl_{1};
  in_addr dst_ip_{};
  in_addr src_ip_{};
  std::uint16_t dst_port_{};
  std::uint16_t src_port_{};
  std::vector<std::uint8_t> pkt_hdr_{};
  std::vector<std::uint8_t> pkt_data_{};
};

#endif
