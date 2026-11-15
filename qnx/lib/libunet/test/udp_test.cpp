#include <netinet/in.h>
#include <arpa/inet.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "rtest/rtest.h"
#include "stdext/expected.h"
#include "ubench/clock.h"
#include "ubench/net.h"
#include "ubench/string.h"

namespace {

// Cache the local interface for these tests.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
in_addr local_interface{};

constexpr auto IFACE_READY = ubench::net::if_flags::MULTICAST |
                             ubench::net::if_flags::UP |
                             ubench::net::if_flags::RUNNING;
constexpr auto IFACE_MASK = IFACE_READY | ubench::net::if_flags::LOOPBACK;

auto is_up(ubench::flags<ubench::net::if_flags> flags) -> bool {
  return (flags & IFACE_MASK) == IFACE_READY;
}

auto find_first_interface() -> in_addr {
  if (local_interface.s_addr == 0) {
    auto interfaces = ubench::net::query_net_interfaces();
    for (const auto& [name, intf] : interfaces) {
      if (is_up(intf.status())) {
        if (!intf.inet().empty()) {
          local_interface = intf.inet()[0].addr();
          return local_interface;
        }
      }
    }
  }
  return local_interface;
}

struct recv_packet {
  sockaddr_in source;
  std::string packet;
};

class udp_receive {
 public:
  udp_receive() = default;
  udp_receive(const udp_receive&) = delete;
  auto operator=(const udp_receive&) -> udp_receive& = delete;
  udp_receive(udp_receive&&) noexcept = default;
  auto operator=(udp_receive&&) noexcept -> udp_receive& = default;
  ~udp_receive() = default;

  auto get_receive_timeout()
      -> stdext::expected<std::chrono::microseconds, int> {
    if (!udp_) return stdext::unexpected{EINVAL};

    timeval tv{};
    socklen_t tvlen = sizeof(tv);
    auto res = getsockopt(udp_, SOL_SOCKET, SO_RCVTIMEO, &tv, &tvlen);
    if (res < 0) return stdext::unexpected{errno};
    unsigned int usec = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return std::chrono::microseconds{usec};
  }

  auto set_receive_timeout(std::chrono::microseconds timeout)
      -> stdext::expected<void, int> {
    if (!udp_) return stdext::unexpected{EINVAL};
    timeval tv = ubench::chrono::duration_to_timeval(timeout);
    auto res = setsockopt(udp_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (res < 0) return stdext::unexpected{errno};
    return {};
  }

  auto open(const sockaddr_in& bind) noexcept -> stdext::expected<void, int> {
    return udp_.open(bind);
  }

  operator bool() const { return udp_; }

  auto close() noexcept -> void { udp_.close(); }

  auto receive() -> stdext::expected<recv_packet, int> {
    if (!udp_) return stdext::unexpected{EINVAL};

    recv_packet p{};
    std::array<char, 64> buffer{};
    socklen_t sl{};
    auto res = recvfrom(udp_, buffer.data(), buffer.size(), 0,
        // Cast required for systems programming sockets.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<sockaddr*>(&p.packet), &sl);
    if (res < 0) return stdext::unexpected{errno};

    p.packet = std::string{buffer.data(), static_cast<std::size_t>(res)};
    return p;
  }

 private:
  ubench::net::udp udp_{};
};

}  // namespace

TEST(ubench_net_udp, udp_default) {
  in_addr localhost{};
  ASSERT_EQ(inet_pton(AF_INET, "127.0.0.1", &localhost), 1);

  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr = localhost;
  localsock.sin_port = htons(37000);

  ubench::net::udp udp{};

  // The socket is not open, so it returns false.
  EXPECT_FALSE(udp);

  // The socket is not open, so the file descriptor is -1.
  ASSERT_EQ(static_cast<int>(udp), -1);

  auto sendbuf = udp.get_sendbuf();
  EXPECT_OPTIONAL_ERROR(sendbuf, EINVAL);

  auto sendbuf_s = udp.set_sendbuf(1400);
  EXPECT_OPTIONAL_ERROR(sendbuf_s, EINVAL);

  auto mcastloop = udp.get_multicast_loopback();
  EXPECT_OPTIONAL_ERROR(mcastloop, EINVAL);

  auto mcastloop_s = udp.set_multicast_loopback(true);
  EXPECT_OPTIONAL_ERROR(mcastloop_s, EINVAL);

  auto mcastreg = udp.multicast_register_interface(localhost);
  EXPECT_OPTIONAL_ERROR(mcastreg, EINVAL);

  auto mcastregs = udp.multicast_register_interface(localsock);
  EXPECT_OPTIONAL_ERROR(mcastregs, EINVAL);

  auto mcastttl = udp.get_multicast_ttl();
  EXPECT_OPTIONAL_ERROR(mcastttl, EINVAL);

  auto mcastttl_s = udp.set_multicast_ttl(2);
  EXPECT_OPTIONAL_ERROR(mcastttl_s, EINVAL);

  // May be used before open.
  auto reuse = udp.get_reuse_addr();
  MAYBE_OPTIONAL_ERROR(reuse, ENOTSUP);

  // May be used before open.
  auto reuse_s = udp.set_reuse_addr(true);
  MAYBE_OPTIONAL_ERROR(reuse_s, ENOTSUP);

  // May be used before open.
  auto reusep = udp.get_reuse_port();
  MAYBE_OPTIONAL_ERROR(reusep, ENOTSUP);

  // May be used before open.
  auto reusep_s = udp.set_reuse_port(true);
  MAYBE_OPTIONAL_ERROR(reusep_s, ENOTSUP);

  auto ipv4hdr = udp.ipv4hdr_length();
  EXPECT_OPTIONAL_ERROR(ipv4hdr, EINVAL);

  auto mcastjoin = udp.multicast_join(localhost, localhost);
  EXPECT_OPTIONAL_ERROR(mcastjoin, EINVAL);

  auto recv = udp.receive();
  EXPECT_OPTIONAL_ERROR(recv, EINVAL);

  auto rtimeout = udp.get_receive_timeout();
  EXPECT_OPTIONAL_ERROR(rtimeout, EINVAL);

  auto rtimeout_s = udp.set_receive_timeout(std::chrono::seconds(1));
  EXPECT_OPTIONAL_ERROR(rtimeout_s, EINVAL);
}

TEST(ubench_net_udp, udp_open_bind_localhost) {
  in_addr localhost{};
  ASSERT_EQ(inet_pton(AF_INET, "127.0.0.1", &localhost), 1);

  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr = localhost;
  localsock.sin_port = htons(37000);

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  // Ensure the socket really is open.
  EXPECT_TRUE(udp);
  EXPECT_NE(static_cast<int>(udp), -1);

  udp.close();
  EXPECT_FALSE(udp);
  EXPECT_EQ(static_cast<int>(udp), -1);
}

TEST(ubench_net_udp, udp_open_bind_connect_localhost) {
  in_addr localhost{};
  ASSERT_EQ(inet_pton(AF_INET, "127.0.0.1", &localhost), 1);

  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr = localhost;
  localsock.sin_port = htons(37000);

  sockaddr_in destsock{};
  destsock.sin_family = AF_INET;
  destsock.sin_addr = localhost;
  destsock.sin_port = htons(37001);

  ubench::net::udp udp{};
  auto ores = udp.open(localsock, destsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  // Ensure the socket really is open.
  EXPECT_TRUE(udp);
  EXPECT_NE(static_cast<int>(udp), -1);

  udp.close();
  EXPECT_FALSE(udp);
  EXPECT_EQ(static_cast<int>(udp), -1);
}

TEST(ubench_net_udp, ipv4hdr_length) {
  in_addr localhost{};
  ASSERT_EQ(inet_pton(AF_INET, "127.0.0.1", &localhost), 1);

  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr = localhost;
  localsock.sin_port = htons(37000);

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  // Ensure the socket really is open.
  EXPECT_TRUE(udp);
  EXPECT_NE(static_cast<int>(udp), -1);

  // With no options, the IPv4 header should be 20 bytes. Users can subtract
  // another 8 bytes for the UDP header data. This, along with the MTU for the
  // interface, can calculate the largest payload without this interface sending
  // and fragmenting the packet.
  EXPECT_EQ(udp.ipv4hdr_length(), 20);
}

TEST(ubench_net_udp, sendbuf) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  auto osb = udp.get_sendbuf();
  ASSERT_OPTIONAL(osb) << ubench::string::perror(osb.error());
  std::cout << "sendbuf = " << *osb << std::endl;

  auto sb_s = udp.set_sendbuf(1420);
  ASSERT_OPTIONAL(sb_s) << ubench::string::perror(sb_s.error());

  auto nsb = udp.get_sendbuf();
  ASSERT_OPTIONAL(nsb) << ubench::string::perror(nsb.error());
  // OSes usually double the buffers.
  EXPECT_GE(*nsb, 1420);
  std::cout << "sendbuf(1420) = " << *nsb << std::endl;
}

TEST(ubench_net_udp, multicast_ttl) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  auto omttl = udp.get_multicast_ttl();
  ASSERT_OPTIONAL(omttl) << ubench::string::perror(omttl.error());

  auto mttl_s = udp.set_multicast_ttl(5);
  ASSERT_OPTIONAL(mttl_s) << ubench::string::perror(mttl_s.error());

  auto nmttl = udp.get_multicast_ttl();
  EXPECT_OPTIONAL_EQ(nmttl, 5) << ubench::string::perror(nmttl.error());
}

TEST(ubench_net_udp, multicast_loopback) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  auto omlb = udp.get_multicast_loopback();
  ASSERT_OPTIONAL(omlb) << ubench::string::perror(omlb.error());

  auto mlb_s = udp.set_multicast_loopback(true);
  ASSERT_OPTIONAL(mlb_s) << ubench::string::perror(mlb_s.error());

  auto nmlb = udp.get_multicast_loopback();
  EXPECT_OPTIONAL_TRUE(nmlb) << ubench::string::perror(nmlb.error());
}

TEST(ubench_net_udp, reuse_addr) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  ubench::net::udp udp{};

  auto ora = udp.get_reuse_addr();
  if (!ora && ora.error() == ENOTSUP) return;
  ASSERT_OPTIONAL(ora) << ubench::string::perror(ora.error());

  auto ra_s = udp.set_reuse_addr(true);
  ASSERT_OPTIONAL(ra_s) << ubench::string::perror(ra_s.error());

  auto ores = udp.open(localsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  auto nra = udp.get_reuse_addr();
  EXPECT_OPTIONAL_TRUE(nra) << ubench::string::perror(nra.error());
}

TEST(ubench_net_udp, reuse_port) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  ubench::net::udp udp{};

  auto orp = udp.get_reuse_port();
  if (!orp && orp.error() == ENOTSUP) return;
  ASSERT_OPTIONAL(orp) << ubench::string::perror(orp.error());

  auto rp_s = udp.set_reuse_port(true);
  ASSERT_OPTIONAL(rp_s) << ubench::string::perror(rp_s.error());

  auto ores = udp.open(localsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  auto nrp = udp.get_reuse_port();
  EXPECT_OPTIONAL_TRUE(nrp) << ubench::string::perror(nrp.error());
}

auto get_packet(const std::string& payload) -> std::vector<std::byte> {
  std::vector<std::byte> bpacket{};
  bpacket.resize(payload.size());
  std::transform(
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      payload.c_str(), payload.c_str() + payload.size(), bpacket.begin(),
      [](const char x) -> auto{ return static_cast<std::byte>(x); });
  return bpacket;
}

auto check_receive(udp_receive& r, std::string exp) -> void {
  auto p = r.receive();
  ASSERT_OPTIONAL(p) << ubench::string::perror(p.error());
  ASSERT_EQ(p->packet, exp);
}

TEST(ubench_net_udp, send_dest_unicast) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  sockaddr_in destsock{};
  destsock.sin_family = AF_INET;
  destsock.sin_addr.s_addr = htonl(0x7F000001);
  destsock.sin_port = htons(37001);

  std::string packet{"T000"};

  udp_receive udpr{};
  udpr.open(destsock);
  udpr.set_receive_timeout(std::chrono::seconds(1));

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  auto sres = udp.send(destsock, packet);
  ASSERT_OPTIONAL_EQ(sres, 4) << ubench::string::perror(sres.error());
  check_receive(udpr, "T000");

  auto sbres = udp.send(destsock, get_packet(packet));
  ASSERT_OPTIONAL_EQ(sbres, 4) << ubench::string::perror(sbres.error());
  check_receive(udpr, "T000");
}

TEST(ubench_net_udp, send_unicast) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  sockaddr_in destsock{};
  destsock.sin_family = AF_INET;
  destsock.sin_addr.s_addr = htonl(0x7F000001);
  destsock.sin_port = htons(37001);

  std::string packet{"T001"};

  udp_receive udpr{};
  udpr.open(destsock);
  udpr.set_receive_timeout(std::chrono::seconds(1));

  ubench::net::udp udp{};
  auto ores = udp.open(localsock, destsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  auto sres = udp.send(packet);
  ASSERT_OPTIONAL_EQ(sres, 4) << ubench::string::perror(sres.error());
  check_receive(udpr, "T001");

  auto sbres = udp.send(get_packet(packet));
  ASSERT_OPTIONAL_EQ(sbres, 4) << ubench::string::perror(sbres.error());
  check_receive(udpr, "T001");
}

TEST(ubench_net_udp, send_unicast_other) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  sockaddr_in destsock{};
  destsock.sin_family = AF_INET;
  destsock.sin_addr.s_addr = htonl(0x7F000001);
  destsock.sin_port = htons(37002);

  sockaddr_in destsock2{};
  destsock2.sin_family = AF_INET;
  destsock2.sin_addr.s_addr = htonl(0x7F000001);
  destsock2.sin_port = htons(37001);

  std::string packet{"T002"};

  udp_receive udpr{};
  udpr.open(destsock2);
  udpr.set_receive_timeout(std::chrono::seconds(1));

  ubench::net::udp udp{};
  auto ores = udp.open(localsock, destsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  auto sres = udp.send(destsock2, packet);
  ASSERT_OPTIONAL_EQ(sres, 4) << ubench::string::perror(sres.error());
  check_receive(udpr, "T002");

  auto sbres = udp.send(destsock2, get_packet(packet));
  ASSERT_OPTIONAL_EQ(sbres, 4) << ubench::string::perror(sbres.error());
  check_receive(udpr, "T002");
}

TEST(ubench_net_udp, send_unicast_no_connect) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  std::string packet{"T003"};

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  // Should fail, as no destination is specific here or in the open.
  auto sres = udp.send(packet);
  ASSERT_FALSE(sres);

  auto sbres = udp.send(get_packet(packet));
  ASSERT_FALSE(sbres);
}

TEST(ubench_net_udp, send_dest_multicast) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr = find_first_interface();
  localsock.sin_port = htons(37000);
  if (localsock.sin_addr.s_addr == 0) {
    GTEST_SKIP() << "No available interfaces for multicast";
  }

  sockaddr_in destsock{};
  destsock.sin_family = AF_INET;
  destsock.sin_addr.s_addr = htonl(0xEFFF2A63);
  destsock.sin_port = htons(37001);

  std::string packet{"T004"};

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  auto mjres = udp.multicast_register_interface(localsock);
  ASSERT_OPTIONAL(mjres) << ubench::string::perror(mjres.error());

  auto mtres = udp.set_multicast_ttl(1);
  ASSERT_OPTIONAL(mtres) << ubench::string::perror(mtres.error());

  auto sres = udp.send(destsock, packet);
  ASSERT_OPTIONAL_EQ(sres, 4) << ubench::string::perror(sres.error());

  auto sbres = udp.send(destsock, get_packet(packet));
  ASSERT_OPTIONAL_EQ(sbres, 4) << ubench::string::perror(sbres.error());
}

TEST(ubench_net_udp, send_multicast) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr = find_first_interface();
  localsock.sin_port = htons(37000);
  if (localsock.sin_addr.s_addr == 0) {
    GTEST_SKIP() << "No available interfaces for multicast";
  }

  sockaddr_in destsock{};
  destsock.sin_family = AF_INET;
  destsock.sin_addr.s_addr = htonl(0xEFFF2A63);
  destsock.sin_port = htons(37001);

  std::string packet{"T005"};

  ubench::net::udp udp{};
  auto ores = udp.open(localsock, destsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  auto mjres = udp.multicast_register_interface(localsock);
  ASSERT_OPTIONAL(mjres) << ubench::string::perror(mjres.error());

  auto mtres = udp.set_multicast_ttl(1);
  ASSERT_OPTIONAL(mtres) << ubench::string::perror(mtres.error());

  auto sres = udp.send(packet);
  ASSERT_OPTIONAL_EQ(sres, 4) << ubench::string::perror(sres.error());

  auto sbres = udp.send(get_packet(packet));
  ASSERT_OPTIONAL_EQ(sbres, 4) << ubench::string::perror(sbres.error());
}

TEST(ubench_net_udp, send_multicast_other) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr = find_first_interface();
  localsock.sin_port = htons(37000);
  if (localsock.sin_addr.s_addr == 0) {
    GTEST_SKIP() << "No available interfaces for multicast";
  }

  sockaddr_in destsock{};
  destsock.sin_family = AF_INET;
  destsock.sin_addr.s_addr = htonl(0xEFFF2A63);
  destsock.sin_port = htons(37001);

  sockaddr_in destsock2{};
  destsock2.sin_family = AF_INET;
  destsock2.sin_addr.s_addr = htonl(0xEFFF2A63);
  destsock2.sin_port = htons(37002);

  std::string packet{"T006"};

  ubench::net::udp udp{};
  auto ores = udp.open(localsock, destsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  auto mjres = udp.multicast_register_interface(localsock);
  ASSERT_OPTIONAL(mjres) << ubench::string::perror(mjres.error());

  auto mtres = udp.set_multicast_ttl(1);
  ASSERT_OPTIONAL(mtres) << ubench::string::perror(mtres.error());

  auto sres = udp.send(destsock2, packet);
  ASSERT_OPTIONAL_EQ(sres, 4) << ubench::string::perror(sres.error());

  auto sbres = udp.send(destsock2, get_packet(packet));
  ASSERT_OPTIONAL_EQ(sbres, 4) << ubench::string::perror(sbres.error());
}

TEST(ubench_net_udp, send_multicast_no_register) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr = find_first_interface();
  localsock.sin_port = htons(37000);
  if (localsock.sin_addr.s_addr == 0) {
    GTEST_SKIP() << "No available interfaces for multicast";
  }

  sockaddr_in destsock{};
  destsock.sin_family = AF_INET;
  destsock.sin_addr.s_addr = htonl(0xEFFF2A64);
  destsock.sin_port = htons(37001);

  std::string packet{"T007"};

  ubench::net::udp udp{};
  auto ores = udp.open(localsock, destsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  auto sres = udp.send(packet);
  ASSERT_OPTIONAL_EQ(sres, 4) << ubench::string::perror(sres.error());

  auto sbres = udp.send(get_packet(packet));
  ASSERT_OPTIONAL_EQ(sbres, 4) << ubench::string::perror(sbres.error());
}

TEST(ubench_net_udp, receive_unicast) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  sockaddr_in destsock{};
  destsock.sin_family = AF_INET;
  destsock.sin_addr.s_addr = htonl(0x7F000001);
  destsock.sin_port = htons(37010);

  std::string packet{"T008"};

  ubench::net::udp rx{};
  auto rxres = rx.open(destsock);
  ASSERT_OPTIONAL(rxres) << ubench::string::perror(rxres.error());
  ASSERT_OPTIONAL(rx.set_receive_timeout(std::chrono::seconds(1)));

  ubench::net::udp tx{};
  auto txres = tx.open(localsock);
  ASSERT_OPTIONAL(txres) << ubench::string::perror(txres.error());

  auto sres = tx.send(destsock, packet);
  ASSERT_OPTIONAL_EQ(sres, 4) << ubench::string::perror(sres.error());

  auto rres = rx.receive();
  ASSERT_OPTIONAL(rres) << ubench::string::perror(rres.error());
  EXPECT_EQ(*rres, get_packet(packet));
}

TEST(ubench_net_udp, receive_into_buffer_keeps_size) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37020);

  sockaddr_in destsock{};
  destsock.sin_family = AF_INET;
  destsock.sin_addr.s_addr = htonl(0x7F000001);
  destsock.sin_port = htons(37021);

  std::string packet{"T008"};

  ubench::net::udp rx{};
  auto rxres = rx.open(destsock);
  ASSERT_OPTIONAL(rxres) << ubench::string::perror(rxres.error());
  ASSERT_OPTIONAL(rx.set_receive_timeout(std::chrono::seconds(1)));

  ubench::net::udp tx{};
  auto txres = tx.open(localsock);
  ASSERT_OPTIONAL(txres) << ubench::string::perror(txres.error());

  auto sres = tx.send(destsock, packet);
  ASSERT_OPTIONAL_EQ(sres, 4) << ubench::string::perror(sres.error());

  std::vector<std::byte> buffer(16, std::byte{0xEE});
  auto rres = rx.receive(buffer);
  ASSERT_OPTIONAL(rres) << ubench::string::perror(rres.error());
  EXPECT_EQ(*rres, 4);
  EXPECT_EQ(buffer.size(), 16U);
  EXPECT_THAT(std::vector<std::byte>(buffer.begin(), buffer.begin() + *rres),
      ::testing::ElementsAreArray(get_packet(packet)));
}

TEST(ubench_net_udp, receive_into_small_buffer_fails) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37020);

  sockaddr_in destsock{};
  destsock.sin_family = AF_INET;
  destsock.sin_addr.s_addr = htonl(0x7F000001);
  destsock.sin_port = htons(37021);

  std::string packet{"T008"};

  ubench::net::udp rx{};
  auto rxres = rx.open(destsock);
  ASSERT_OPTIONAL(rxres) << ubench::string::perror(rxres.error());
  ASSERT_OPTIONAL(rx.set_receive_timeout(std::chrono::seconds(1)));

  ubench::net::udp tx{};
  auto txres = tx.open(localsock);
  ASSERT_OPTIONAL(txres) << ubench::string::perror(txres.error());

  auto sres = tx.send(destsock, packet);
  ASSERT_OPTIONAL_EQ(sres, 4) << ubench::string::perror(sres.error());

  std::vector<std::byte> buffer(2);
  auto rres = rx.receive(buffer);
  ASSERT_OPTIONAL_ERROR(rres, EMSGSIZE);
  EXPECT_EQ(buffer.size(), 2U);
}

TEST(ubench_net_udp, receive_unicast_blocked) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37016);

  sockaddr_in destsock{};
  destsock.sin_family = AF_INET;
  destsock.sin_addr.s_addr = htonl(0x7F000001);
  destsock.sin_port = htons(37015);

  std::string packet{"T010"};

  ubench::net::udp rx{};
  auto rxres = rx.open(destsock);
  ASSERT_OPTIONAL(rxres) << ubench::string::perror(rxres.error());
  ASSERT_OPTIONAL(rx.set_receive_timeout(std::chrono::seconds(2)));

  ubench::net::udp tx{};
  auto txres = tx.open(localsock);
  ASSERT_OPTIONAL(txres) << ubench::string::perror(txres.error());

  std::optional<stdext::expected<std::vector<std::byte>, int>> rres;
  std::thread receiver([&rx, &rres]() -> void { rres = rx.receive(); });

  // Give the receiver thread time to enter receive() before sending.
  std::this_thread::sleep_for(std::chrono::milliseconds{100});
  auto sres = tx.send(destsock, packet);

  receiver.join();
  ASSERT_OPTIONAL_EQ(sres, 4) << ubench::string::perror(sres.error());
  ASSERT_OPTIONAL(rres);
  ASSERT_OPTIONAL(*rres) << ubench::string::perror((*rres).error());
  EXPECT_EQ(**rres, get_packet(packet));
}

TEST(ubench_net_udp, receive_multicast) {
  in_addr local = find_first_interface();
  if (local.s_addr == 0) {
    GTEST_SKIP() << "No available interfaces for multicast";
  }

  sockaddr_in bindsock{};
  bindsock.sin_family = AF_INET;
  bindsock.sin_addr.s_addr = htonl(INADDR_ANY);
  bindsock.sin_port = htons(37011);

  sockaddr_in groupsock{};
  groupsock.sin_family = AF_INET;
  groupsock.sin_addr.s_addr = htonl(0xEFFF2A63);
  groupsock.sin_port = htons(37011);

  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr = local;
  localsock.sin_port = htons(37012);

  std::string packet{"T009"};

  ubench::net::udp rx{};
  auto rxres = rx.open(bindsock);
  ASSERT_OPTIONAL(rxres) << ubench::string::perror(rxres.error());
  ASSERT_OPTIONAL(rx.set_receive_timeout(std::chrono::seconds(1)));

  auto jres = rx.multicast_join(groupsock.sin_addr);
  ASSERT_OPTIONAL(jres) << ubench::string::perror(jres.error());

  ubench::net::udp tx{};
  auto txres = tx.open(localsock, groupsock);
  ASSERT_OPTIONAL(txres) << ubench::string::perror(txres.error());

  auto sres = tx.send(packet);
  ASSERT_OPTIONAL_EQ(sres, 4) << ubench::string::perror(sres.error());

  auto rres = rx.receive();
  ASSERT_OPTIONAL(rres) << ubench::string::perror(rres.error());
  EXPECT_EQ(*rres, get_packet(packet));
}

TEST(ubench_net_udp, multicast_join_unicast) {
  in_addr localhost{};
  ASSERT_EQ(inet_pton(AF_INET, "127.0.0.1", &localhost), 1);

  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr = localhost;
  localsock.sin_port = htons(37013);

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  // Only multicast group addresses can be joined.
  auto jres = udp.multicast_join(localhost, localhost);
  EXPECT_OPTIONAL_ERROR(jres, EINVAL);
}

TEST(ubench_net_udp, receive_timeout) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37014);

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_OPTIONAL(ores) << ubench::string::perror(ores.error());

  auto ts = udp.set_receive_timeout(std::chrono::milliseconds{250});
  ASSERT_OPTIONAL(ts) << ubench::string::perror(ts.error());

  auto nt = udp.get_receive_timeout();
  ASSERT_OPTIONAL_EQ(nt, std::chrono::milliseconds{250});

  auto nts = udp.set_receive_timeout(std::chrono::microseconds{-1});
  EXPECT_OPTIONAL_ERROR(nts, EINVAL);

  // Nothing is sent, so the receive times out.
  auto rres = udp.receive();
  ASSERT_FALSE(rres);
  EXPECT_TRUE(rres.error() == EAGAIN || rres.error() == EWOULDBLOCK);
}

// Manual test: receives packets from an externally running multicast sender.
// Run with: libunet_test --gtest_also_run_disabled_tests
// --gtest_filter='ubench_net_udp.DISABLED_receive_external_multicast'
//
// Designed to work with the "sdd" program in this repository, which sends the
// multicast messages.
TEST(ubench_net_udp, DISABLED_receive_external_multicast) {
  constexpr std::uint16_t port = 52599;
  constexpr std::size_t packets_to_receive = 10;

  sockaddr_in bindsock{};
  bindsock.sin_family = AF_INET;
  bindsock.sin_addr.s_addr = htonl(INADDR_ANY);
  bindsock.sin_port = htons(port);

  in_addr group{};
  group.s_addr = htonl(0xEFFF2A63);  // 239.255.42.99

  ubench::net::udp rx{};
  ASSERT_OPTIONAL(rx.set_reuse_addr(true));
  auto rxres = rx.open(bindsock);
  ASSERT_OPTIONAL(rxres) << ubench::string::perror(rxres.error());
  ASSERT_OPTIONAL(rx.set_receive_timeout(std::chrono::seconds(30)));

  auto jres = rx.multicast_join(group);
  ASSERT_OPTIONAL(jres) << ubench::string::perror(jres.error());

  for (std::size_t i = 0; i < packets_to_receive; ++i) {
    auto rres = rx.receive();
    ASSERT_OPTIONAL(rres) << ubench::string::perror(rres.error());
    std::string text;
    for (std::byte b : *rres) text.push_back(static_cast<char>(b));
    std::cout << std::string_view{text} << '\n';
  }
}
