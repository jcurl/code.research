#include "sdd/local_computer_service.h"

#include <netinet/in.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ubench/net.h"

namespace sdd::core {

namespace {

// Bounds how long stop() takes to be seen by the receive thread.
constexpr auto receive_timeout = std::chrono::milliseconds{100};

// Placeholder until the discovery payload is decoded.
auto parse_computer([[maybe_unused]] std::string_view json) -> computer {
  computer packet{};
  packet.id = -1;
  packet.last_seen = std::time(nullptr);
  return packet;
}

}  // namespace

local_computer_service::local_computer_service(
    local_computer_service_config config)
    : config_{std::move(config)} {
  computers_.reserve(16);
}

local_computer_service::~local_computer_service() {
  stop();
  if (worker_.joinable()) {
    worker_.join();
  }
}

[[nodiscard]] auto local_computer_service::start()
    -> stdext::expected<void, int> {
  if (running_ && !stop_requested_) return {};

  // A thread that is stopping must exit before a new one is started.
  if (worker_.joinable()) {
    worker_.join();
  }

  auto socket = open_socket();
  if (!socket) {
    last_error_ = socket.error();
    return stdext::unexpected{socket.error()};
  }

  stop_requested_ = false;
  last_error_ = 0;
  running_ = true;
  worker_ = std::thread{&local_computer_service::run, this, std::move(*socket)};
  return {};
}

auto local_computer_service::open_socket()
    -> stdext::expected<ubench::net::udp, int> {
  auto group = ubench::net::parse_sockaddr(config_.multicast_group);
  if (!group || !ubench::net::is_multicast(group->sin_addr))
    return stdext::unexpected{EINVAL};

  sockaddr_in local{};
  local.sin_family = AF_INET;
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  local.sin_port = htons(config_.port);

  ubench::net::udp socket{};
  if (auto res = socket.set_reuse_addr(true); !res)
    return stdext::unexpected{res.error()};

  if (auto res = socket.open(local); !res)
    return stdext::unexpected{res.error()};

  if (auto res = socket.multicast_join(group->sin_addr); !res)
    return stdext::unexpected{res.error()};

  if (auto res = socket.set_receive_timeout(receive_timeout); !res)
    return stdext::unexpected{res.error()};

  return socket;
}

auto local_computer_service::run(ubench::net::udp socket) -> void {
  int error = 0;
  // Largest UDP payload over IPv4, so no datagram is rejected for size.
  std::vector<std::byte> buffer(std::numeric_limits<std::uint16_t>::max());
  while (!stop_requested_) {
    auto nbytes = socket.receive(buffer);
    if (!nbytes) {
      // A timeout lets the loop check stop_requested_.
      if (nbytes.error() == EAGAIN || nbytes.error() == EWOULDBLOCK) {
        continue;
      }
      error = nbytes.error();
      break;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    std::string_view text{reinterpret_cast<const char*>(buffer.data()),
        static_cast<std::size_t>(*nbytes)};
    computer_ping_received(parse_computer(text));
  }

  socket.close();
  last_error_ = error;
  running_ = false;
  observers_.notify(
      [error](computer_observer& o) -> void { o.on_terminated(error); });
}

auto local_computer_service::computer_ping_received(const computer& packet)
    -> int {
  auto it = std::find_if(computers_.begin(), computers_.end(),
      [&packet](const computer& c) -> bool {
        return c.hostname == packet.hostname && c.domain == packet.domain;
      });

  int id = -1;
  if (it == computers_.end()) {
    computer added = packet;
    id = static_cast<int>(computers_.size());
    added.id = id;
    computers_.push_back(std::move(added));
    it = computers_.end() - 1;
  } else {
    id = it->id;
    *it = packet;
    it->id = id;
  }

  observers_.notify(
      [&it](computer_observer& o) -> void { o.on_computer(*it); });
  return id;
}

}  // namespace sdd::core
