#include "sdd/local_computer_service.h"

#include <vector>

#include <gtest/gtest.h>

#include "sdd/computer.h"
#include "sdd/computer_observer.h"

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

  void on_computer(const computer& c) override { seen.push_back(c); }

  std::vector<computer> seen;
};

auto make_computer(const char* hostname, const char* domain, time_t last_seen)
    -> computer {
  return computer{-1, last_seen, hostname, domain, {}};
}

}  // namespace

TEST(local_computer_service, sequential_ids) {
  local_computer_service service;

  EXPECT_EQ(
      service.computer_ping_received(make_computer("host1", "lan", 1)), 0);
  EXPECT_EQ(
      service.computer_ping_received(make_computer("host2", "lan", 2)), 1);
}

TEST(local_computer_service, update_existing_id) {
  local_computer_service service;
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
  local_computer_service service;

  EXPECT_EQ(
      service.computer_ping_received(make_computer("host1", "lan", 1)), 0);
  EXPECT_EQ(
      service.computer_ping_received(make_computer("host1", "wan", 2)), 1);
}

TEST(local_computer_service, notifies_observers) {
  local_computer_service service;
  recording_observer observer;
  auto subscription = service.subscribe(observer);

  service.computer_ping_received(make_computer("host1", "lan", 1));
  service.computer_ping_received(make_computer("host1", "lan", 2));

  ASSERT_EQ(observer.seen.size(), 2U);
  EXPECT_EQ(observer.seen[0].id, 0);
  EXPECT_EQ(observer.seen[1].id, 0);
}

}  // namespace sdd::core