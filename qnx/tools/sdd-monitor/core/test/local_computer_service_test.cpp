#include "sdd/local_computer_service.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <string_view>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "rtest/rtest.h"
#include "sdd/computer.h"
#include "sdd/computer_observer.h"
#include "sdd/local_computer_service_config.h"
#include "ubench/net.h"
#include "packet_samples.h"

namespace sdd::core {

namespace {

class recording_observer : public computer_observer {
 public:
  recording_observer() = default;
  recording_observer(const recording_observer&) = delete;
  auto operator=(const recording_observer&) -> recording_observer& = delete;
  recording_observer(recording_observer&&) = delete;
  auto operator=(recording_observer&&) -> recording_observer& = delete;
  ~recording_observer() override = default;

  void on_computer(const data::computer& c) override {
    seen.push_back(c);
    computer_count++;
  }

  void on_terminated(int error) override {
    terminated_error = error;
    terminated = true;
  }

  std::vector<data::computer> seen;
  std::atomic<int> computer_count{0};
  std::atomic<int> terminated_error{-1};
  std::atomic<bool> terminated{false};
};

/// @brief Builds a computer with no interfaces and an unassigned id.
/// @param hostname The hostname.
/// @param domain The domain name.
/// @param last_seen The Unix timestamp of the last update.
/// @return The computer.
auto make_computer(const char* hostname, const char* domain, time_t last_seen)
    -> data::computer {
  return data::computer{-1, last_seen, hostname, domain, {}};
}

}  // namespace

TEST(local_computer_service, sequential_ids) {
  local_computer_service service{local_computer_service_config{}};

  EXPECT_EQ(
      service.computer_ping_received(make_computer("host1", "lan", 1)), 0);
  EXPECT_EQ(
      service.computer_ping_received(make_computer("host2", "lan", 2)), 1);
}

TEST(local_computer_service, update_existing_id) {
  local_computer_service service{local_computer_service_config{}};
  recording_observer observer;
  auto subscription = service.subscribe(observer);

  EXPECT_EQ(
      service.computer_ping_received(make_computer("host1", "lan", 1)), 0);
  EXPECT_EQ(
      service.computer_ping_received(make_computer("host2", "lan", 2)), 1);
  EXPECT_EQ(
      service.computer_ping_received(make_computer("host1", "lan", 3)), 0);

  ASSERT_EQ(observer.seen.size(), 3U);
  EXPECT_EQ(observer.seen[2].id, 0);
  EXPECT_EQ(observer.seen[2].hostname, "host1");
  EXPECT_EQ(observer.seen[2].last_seen, 3);
}

TEST(local_computer_service, different_domain_new_id) {
  local_computer_service service{local_computer_service_config{}};

  EXPECT_EQ(
      service.computer_ping_received(make_computer("host1", "lan", 1)), 0);
  EXPECT_EQ(
      service.computer_ping_received(make_computer("host1", "wan", 2)), 1);
}

TEST(local_computer_service, notifies_observers) {
  local_computer_service service{local_computer_service_config{}};
  recording_observer observer;
  auto subscription = service.subscribe(observer);

  service.computer_ping_received(make_computer("host1", "lan", 1));
  service.computer_ping_received(make_computer("host1", "lan", 2));

  ASSERT_EQ(observer.seen.size(), 2U);
  EXPECT_EQ(observer.seen[0].id, 0);
  EXPECT_EQ(observer.seen[1].id, 0);
}

namespace {

// Polls until the predicate holds or the timeout expires.
template <typename Predicate>
auto wait_until(Predicate predicate, std::chrono::milliseconds timeout)
    -> bool {
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (!predicate()) {
    if (std::chrono::steady_clock::now() > deadline) {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
  }
  return true;
}

auto loopback(std::uint16_t port) -> sockaddr_in {
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(port);
  return addr;
}

}  // namespace

TEST(local_computer_service, start_receives_unicast_and_stop_terminates) {
  constexpr std::uint16_t port = 37050;
  local_computer_service_config config;
  config.port = port;
  local_computer_service service{config};
  recording_observer observer;
  auto subscription = service.subscribe(observer);

  ASSERT_TRUE(service.start());
  ASSERT_TRUE(service.is_running());

  ubench::net::udp sender;
  ASSERT_TRUE(sender.open(loopback(0)));
  ASSERT_TRUE(sender.send(loopback(port), test::computer_packet_json));

  EXPECT_TRUE(
      wait_until([&observer]() -> bool { return observer.computer_count > 0; },
          std::chrono::seconds{2}));

  service.stop();
  EXPECT_TRUE(
      wait_until([&observer]() -> bool { return observer.terminated.load(); },
          std::chrono::seconds{2}));
  EXPECT_EQ(observer.terminated_error.load(), 0);
  auto state = service.is_running();
  ASSERT_OPTIONAL_ERROR(state, 0);
}

TEST(local_computer_service, parses_packet_and_keeps_known_interfaces) {
  constexpr std::uint16_t port = 37052;
  local_computer_service_config config;
  config.port = port;
  local_computer_service service{config};
  recording_observer observer;
  auto subscription = service.subscribe(observer);

  ASSERT_TRUE(service.start());

  ubench::net::udp sender;
  ASSERT_TRUE(sender.open(loopback(0)));
  ASSERT_TRUE(sender.send(loopback(port), test::computer_packet_json));
  ASSERT_TRUE(sender.send(loopback(port), test::computer_packet_json));

  EXPECT_TRUE(
      wait_until([&observer]() -> bool { return observer.computer_count >= 2; },
          std::chrono::seconds{2}));
  service.stop();
  EXPECT_TRUE(
      wait_until([&observer]() -> bool { return observer.terminated.load(); },
          std::chrono::seconds{2}));

  ASSERT_EQ(observer.seen.size(), 2U);
  for (const auto& c : observer.seen) {
    EXPECT_EQ(c.id, 0);
    EXPECT_EQ(c.hostname, "rotorua");
    EXPECT_EQ(c.domain, "home.lan");
    ASSERT_EQ(c.interfaces.size(), 2U);
    EXPECT_EQ(c.interfaces.at("enp4s0").mac, "02:00:00:00:00:01");
    EXPECT_EQ(c.interfaces.at("enp4s0").mtu, 1500U);
    EXPECT_EQ(c.interfaces.at("enp4s0").ipv4,
        std::vector<std::string>{"192.168.1.110/24"});
    EXPECT_EQ(c.interfaces.at("lo").mtu, 65536U);
  }
}

TEST(local_computer_service, drops_invalid_packet_and_defaults_bad_mtu) {
  constexpr std::uint16_t port = 37053;
  local_computer_service_config config;
  config.port = port;
  local_computer_service service{config};
  recording_observer observer;
  auto subscription = service.subscribe(observer);

  ASSERT_TRUE(service.start());

  // The invalid packet is received first, so it is processed first.
  ubench::net::udp sender;
  ASSERT_TRUE(sender.open(loopback(0)));
  ASSERT_TRUE(sender.send(loopback(port), test::computer_bad_mac_json));
  ASSERT_TRUE(sender.send(loopback(port), test::computer_bad_mtu_json));

  EXPECT_TRUE(
      wait_until([&observer]() -> bool { return observer.computer_count > 0; },
          std::chrono::seconds{2}));
  service.stop();
  EXPECT_TRUE(
      wait_until([&observer]() -> bool { return observer.terminated.load(); },
          std::chrono::seconds{2}));

  ASSERT_EQ(observer.seen.size(), 1U);
  EXPECT_EQ(observer.seen[0].hostname, "mtu");
  EXPECT_EQ(observer.seen[0].interfaces.at("eth0").mac, "02:00:00:00:00:02");
  EXPECT_EQ(observer.seen[0].interfaces.at("eth0").mtu, 0U);
}

TEST(local_computer_service, destructor_without_stop_terminates) {
  constexpr std::uint16_t port = 37051;
  local_computer_service_config config;
  config.port = port;
  {
    local_computer_service service{config};
    ASSERT_TRUE(service.start());
    ASSERT_TRUE(service.is_running());
  }

  // The thread has been joined and its socket closed, so the port is free.
  ubench::net::udp probe;
  EXPECT_TRUE(probe.open(loopback(port)));
}

TEST(local_computer_service, start_rejects_invalid_group) {
  local_computer_service_config config;
  config.multicast_group = "not-an-address";
  local_computer_service service{config};

  auto res = service.start();
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error(), EINVAL);
  auto state = service.is_running();
  ASSERT_OPTIONAL_ERROR(state, EINVAL);
}

}  // namespace sdd::core