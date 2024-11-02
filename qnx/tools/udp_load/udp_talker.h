#ifndef BENCHMARK_UDP_TALKER_H
#define BENCHMARK_UDP_TALKER_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "busy_measurement.h"
#include "config.h"

/// @brief The class to instantiate when testing.
enum class send_mode {
  mode_sendto,   //< Use sendto() for sending packets.
  mode_sendmmsg  //< use sendmmsg() for sending packets.
};

/// @brief Executes a period of time in idle for background CPU usage
/// measurement.
///
/// @param duration The duration for the idle measurement.
///
/// @return The success of the measurement. If the idle time cannot be
/// measured, false is returned immediately.
auto idle_test(std::chrono::milliseconds duration) noexcept
    -> std::optional<busy_measurement>;

/// @brief Results of a measurement.
struct udp_results {
  /// @brief The number of packets sent during the test.
  std::uint32_t packets_sent;

  /// @brief The number of packets dropped / not sent, during the test.
  std::uint32_t packets_expected;

  /// @brief The amount of time the benchmark is sleeping.
  std::chrono::milliseconds wait_time;

  /// @brief The amount of time spent sending traffic.
  std::chrono::milliseconds send_time;
};

/// @brief Base class for a UDP talker.
class udp_talker {
 public:
  udp_talker() noexcept = default;
  udp_talker(const udp_talker& other) = delete;
  auto operator=(const udp_talker& other) -> udp_talker& = delete;
  udp_talker(udp_talker&& other) = delete;
  auto operator=(udp_talker&& other) -> udp_talker& = delete;
  virtual ~udp_talker() = default;

  /// @brief Check if this implementation is valid for this platform.
  ///
  /// Not all platforms support all methods for doing benchmarks. By
  /// instantiating this object and testing if it's enabled, we can test if it
  /// usable or not.
  ///
  /// @return true if this benchmark is usable, false otherwise.
  virtual auto is_supported() noexcept -> bool { return false; }

  /// @brief Set the address (IPv4) to bind to locally.
  ///
  /// Sets the source address for the UDP packets to the address
  /// specified. The SO_REUSEADDR and SO_REUSEPORT will be set, that
  /// multiple talkers can be created on the same source interface.
  ///
  /// @param addr The source address to bind to.
  ///
  /// @param port The source port to bind to.
  ///
  /// @return Returns true, if setting the source was successful.
  auto set_source_addr(const struct sockaddr_in& addr) noexcept -> bool;

  /// @brief Set the address (IPv4) to send to.
  ///
  /// Sets the destination address for the UDP packets. May be a unicast
  /// address or a multicast address.
  ///
  /// @param addr The destination address to send to.
  ///
  /// @param port The destination port to send to.
  ///
  /// @return Returns true, if setting the destination was successful.
  auto set_dest_addr(const struct sockaddr_in& addr) noexcept -> bool;

  /// @brief Sets the shaping parameters for the talker for sending.
  ///
  /// Sets the shaping parameters for sending traffic. Shaping is
  /// attempted for the window of time defined by tne number of slots and
  /// the width of each slot, with the number of packets over all slots.
  ///
  /// @param slots The number of slots. Must be one or more.
  ///
  /// @param width The width of each slot, in milliseconds. Must be one or
  /// more.
  ///
  /// @param packets The number of packets to send over the window defined
  /// by slots * width.
  ///
  /// @return Returns true if setting the shaping parameters succeeded,
  /// false otherwise.
  auto set_shaping(std::uint16_t slots, std::uint16_t width,
                   std::uint32_t packets, std::uint16_t pkt_size) noexcept
      -> bool;

  /// @brief Initialise the benchmark for testing.
  ///
  /// The initialisation step can be run first, and should be run without any
  /// timing measurements.
  ///
  /// @return true if initialisation succeeded, false otherwise.
  auto init() noexcept -> bool;

  /// @brief Runs the simulation.
  ///
  /// @param duration The duration for the idle measurement. It is rounded to
  /// the nearest slot duration.
  ///
  /// @return The success of the simulation.
  auto run(std::chrono::milliseconds duration) noexcept
      -> std::optional<udp_results>;

 protected:
  auto virtual init_packets(const struct sockaddr_in& source,
                            const struct sockaddr_in& dest,
                            std::uint16_t pkt_size) noexcept -> bool;
  auto virtual send_packets(std::uint16_t count) noexcept -> std::uint16_t;

 private:
  struct sockaddr_in source_ {};
  struct sockaddr_in dest_ {};
  std::uint16_t slots_{0};
  std::uint16_t width_{0};
  std::uint16_t size_{0};
  std::uint32_t packets_{0};
  bool init_{false};
};

/// @brief An intermediate class not for direct instantiation.
///
/// This class provides common initialisation code needed for using BSD socket
/// API for the udp_talker.
class udp_talker_bsd : public udp_talker {
 public:
  explicit udp_talker_bsd() = default;
  ~udp_talker_bsd() override;

 protected:
  auto init_packets(const struct sockaddr_in& source,
                    const struct sockaddr_in& dest,
                    std::uint16_t pkt_size) noexcept -> bool override;

 protected:
  int socket_fd_{-1};
  struct sockaddr_in source_ {};
  struct sockaddr_in dest_ {};
};

/// @brief Send packets using the sendto() BSD socket call.
class udp_talker_sendto final : public udp_talker_bsd {
 public:
  explicit udp_talker_sendto() = default;

  auto is_supported() noexcept -> bool override { return true; }

 protected:
  auto init_packets(const struct sockaddr_in& source,
                    const struct sockaddr_in& dest,
                    std::uint16_t pkt_size) noexcept -> bool override;
  auto send_packets(std::uint16_t count) noexcept -> std::uint16_t override;

 private:
  std::vector<std::uint8_t> buffer_{};
};

#if HAVE_SENDMMSG
/// @brief Send packets using the sendmmsg() socket call.
///
/// The sendmmsg() call allows to send multiple packets in one system call,
/// allowing for the reduction of overhead for multiple packets in one slot.
class udp_talker_sendmmsg final : public udp_talker_bsd {
 public:
  explicit udp_talker_sendmmsg() = default;

  auto is_supported() noexcept -> bool override { return true; }

 protected:
  auto init_packets(const struct sockaddr_in& source,
                    const struct sockaddr_in& dest,
                    std::uint16_t pkt_size) noexcept -> bool override;
  auto send_packets(std::uint16_t count) noexcept -> std::uint16_t override;

 private:
  std::vector<std::vector<std::uint8_t>> eth_packets_{};
  std::vector<struct iovec> msgvec_{};
  std::vector<struct mmsghdr> msgpool_{};
};
#else
// The sendmmsg() is not supported.
using udp_talker_sendmmsg = udp_talker;
#endif

/// @brief Create the benchmark class from the mode.
auto make_udp_talker(send_mode mode) -> std::unique_ptr<udp_talker>;

#endif
