#include <netinet/in.h>
#include <arpa/inet.h>

#include <cerrno>
#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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
  ASSERT_FALSE(sendbuf.has_value());
  if (!sendbuf) {
    EXPECT_EQ(sendbuf.error(), EINVAL);
  }

  auto sendbuf_s = udp.set_sendbuf(1400);
  ASSERT_FALSE(sendbuf_s.has_value());
  if (!sendbuf_s) {
    EXPECT_EQ(sendbuf_s.error(), EINVAL);
  }

  auto mcastloop = udp.get_multicast_loopback();
  ASSERT_FALSE(mcastloop.has_value());
  if (!mcastloop) {
    EXPECT_EQ(mcastloop.error(), EINVAL);
  }

  auto mcastloop_s = udp.set_multicast_loopback(true);
  ASSERT_FALSE(mcastloop_s.has_value());
  if (!mcastloop_s) {
    EXPECT_EQ(mcastloop_s.error(), EINVAL);
  }

  auto mcastreg = udp.multicast_register_interface(localhost);
  ASSERT_FALSE(mcastreg.has_value());
  if (!mcastreg) {
    EXPECT_EQ(mcastreg.error(), EINVAL);
  }

  auto mcastregs = udp.multicast_register_interface(localsock);
  ASSERT_FALSE(mcastregs.has_value());
  if (!mcastregs) {
    EXPECT_EQ(mcastreg.error(), EINVAL);
  }

  auto mcastttl = udp.get_multicast_ttl();
  ASSERT_FALSE(mcastttl.has_value());
  if (!mcastttl) {
    EXPECT_EQ(mcastttl.error(), EINVAL);
  }

  auto mcastttl_s = udp.set_multicast_ttl(2);
  ASSERT_FALSE(mcastttl_s.has_value());
  if (!mcastttl_s) {
    EXPECT_EQ(mcastttl_s.error(), EINVAL);
  }

  auto reuse = udp.get_reuse_addr();
  ASSERT_FALSE(reuse.has_value());
  if (!reuse) {
    EXPECT_EQ(reuse.error(), EINVAL);
  }

  auto reuse_s = udp.set_reuse_addr(true);
  ASSERT_FALSE(reuse_s.has_value());
  if (!reuse_s) {
    EXPECT_EQ(reuse_s.error(), EINVAL);
  }

  auto reusep = udp.get_reuse_port();
  ASSERT_FALSE(reusep.has_value());
  if (!reusep) {
    EXPECT_EQ(reusep.error(), EINVAL);
  }

  auto reusep_s = udp.set_reuse_port(true);
  ASSERT_FALSE(reusep_s.has_value());
  if (!reusep_s) {
    EXPECT_EQ(reusep_s.error(), EINVAL);
  }

  auto ipv4hdr = udp.ipv4hdr_length();
  ASSERT_FALSE(ipv4hdr.has_value());
  if (!ipv4hdr) {
    EXPECT_EQ(ipv4hdr.error(), EINVAL);
  }
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
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

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
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

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
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

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
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

  auto osb = udp.get_sendbuf();
  EXPECT_TRUE(osb) << ubench::string::perror(osb.error());
  std::cout << "sendbuf = " << *osb << std::endl;

  auto sb_s = udp.set_sendbuf(1420);
  EXPECT_TRUE(sb_s) << ubench::string::perror(sb_s.error());

  if (osb && sb_s) {
    auto nsb = udp.get_sendbuf();
    EXPECT_TRUE(nsb) << ubench::string::perror(nsb.error());
    if (nsb) {
      // OSes usually double the buffers.
      EXPECT_GE(*nsb, 1420);
      std::cout << "sendbuf(1420) = " << *nsb << std::endl;
    }
  }
}

TEST(ubench_net_udp, multicast_ttl) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

  auto omttl = udp.get_multicast_ttl();
  EXPECT_TRUE(omttl) << ubench::string::perror(omttl.error());
  ;

  auto mttl_s = udp.set_multicast_ttl(5);
  EXPECT_TRUE(mttl_s) << ubench::string::perror(mttl_s.error());

  if (omttl && mttl_s) {
    auto nmttl = udp.get_multicast_ttl();
    EXPECT_TRUE(nmttl) << ubench::string::perror(nmttl.error());
    if (nmttl) {
      EXPECT_EQ(*nmttl, 5);
    }
  }
}

TEST(ubench_net_udp, multicast_loopback) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

  auto omlb = udp.get_multicast_loopback();
  EXPECT_TRUE(omlb) << ubench::string::perror(omlb.error());

  auto mlb_s = udp.set_multicast_loopback(true);
  EXPECT_TRUE(mlb_s) << ubench::string::perror(mlb_s.error());

  if (omlb && mlb_s) {
    auto nmlb = udp.get_multicast_loopback();
    EXPECT_TRUE(nmlb) << ubench::string::perror(nmlb.error());
    if (nmlb) {
      EXPECT_TRUE(*nmlb);
    }
  }
}

TEST(ubench_net_udp, reuse_addr) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

  auto ora = udp.get_reuse_addr();
  EXPECT_TRUE(ora) << ubench::string::perror(ora.error());

  auto ra_s = udp.set_reuse_addr(true);
  EXPECT_TRUE(ra_s) << ubench::string::perror(ra_s.error());

  if (ora && ra_s) {
    auto nra = udp.get_reuse_addr();
    EXPECT_TRUE(nra) << ubench::string::perror(nra.error());
    if (nra) {
      EXPECT_TRUE(*nra);
    }
  }
}

TEST(ubench_net_udp, reuse_port) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

  auto orp = udp.get_reuse_port();
  EXPECT_TRUE(orp) << ubench::string::perror(orp.error());

  auto rp_s = udp.set_reuse_port(true);
  EXPECT_TRUE(rp_s) << ubench::string::perror(rp_s.error());

  if (orp && rp_s) {
    auto nrp = udp.get_reuse_port();
    EXPECT_TRUE(nrp) << ubench::string::perror(nrp.error());
    if (nrp) {
      EXPECT_TRUE(*nrp);
    }
  }
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

  std::string packet{"Test"};

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

  auto sres = udp.send(destsock, packet);
  ASSERT_TRUE(sres) << ubench::string::perror(sres.error());
  ASSERT_EQ(*sres, 4);
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

  std::string packet{"Test"};

  ubench::net::udp udp{};
  auto ores = udp.open(localsock, destsock);
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

  auto sres = udp.send(packet);
  ASSERT_TRUE(sres) << ubench::string::perror(sres.error());
  ASSERT_EQ(*sres, 4);
}

TEST(ubench_net_udp, send_unicast_other) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  sockaddr_in destsock{};
  destsock.sin_family = AF_INET;
  destsock.sin_addr.s_addr = htonl(0x7F000001);
  destsock.sin_port = htons(37001);

  sockaddr_in destsock2{};
  destsock2.sin_family = AF_INET;
  destsock2.sin_addr.s_addr = htonl(0x7F000001);
  destsock2.sin_port = htons(37002);

  std::string packet{"Test"};

  ubench::net::udp udp{};
  auto ores = udp.open(localsock, destsock);
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

  auto sres = udp.send(destsock2, packet);
  ASSERT_TRUE(sres) << ubench::string::perror(sres.error());
  ASSERT_EQ(*sres, 4);
}

TEST(ubench_net_udp, send_unicast_no_connect) {
  sockaddr_in localsock{};
  localsock.sin_family = AF_INET;
  localsock.sin_addr.s_addr = htonl(0x7F000001);
  localsock.sin_port = htons(37000);

  std::string packet{"Test"};

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

  // Should fail, as no destination is specific here or in the open.
  auto sres = udp.send(packet);
  ASSERT_FALSE(sres);
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

  std::string packet{"Test"};

  ubench::net::udp udp{};
  auto ores = udp.open(localsock);
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

  auto mjres = udp.multicast_register_interface(localsock);
  ASSERT_TRUE(mjres) << ubench::string::perror(mjres.error());

  auto mtres = udp.set_multicast_ttl(1);
  ASSERT_TRUE(mtres) << ubench::string::perror(mtres.error());

  auto sres = udp.send(destsock, packet);
  ASSERT_TRUE(sres) << ubench::string::perror(sres.error());
  ASSERT_EQ(*sres, 4);
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

  std::string packet{"Test"};

  ubench::net::udp udp{};
  auto ores = udp.open(localsock, destsock);
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

  auto mjres = udp.multicast_register_interface(localsock);
  ASSERT_TRUE(mjres) << ubench::string::perror(mjres.error());

  auto mtres = udp.set_multicast_ttl(1);
  ASSERT_TRUE(mtres) << ubench::string::perror(mtres.error());

  auto sres = udp.send(packet);
  ASSERT_TRUE(sres) << ubench::string::perror(sres.error());
  ASSERT_EQ(*sres, 4);
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

  std::string packet{"Test"};

  ubench::net::udp udp{};
  auto ores = udp.open(localsock, destsock);
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

  auto mjres = udp.multicast_register_interface(localsock);
  ASSERT_TRUE(mjres) << ubench::string::perror(mjres.error());

  auto mtres = udp.set_multicast_ttl(1);
  ASSERT_TRUE(mtres) << ubench::string::perror(mtres.error());

  auto sres = udp.send(destsock2, packet);
  ASSERT_TRUE(sres) << ubench::string::perror(sres.error());
  ASSERT_EQ(*sres, 4);
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

  std::string packet{"Test"};

  ubench::net::udp udp{};
  auto ores = udp.open(localsock, destsock);
  ASSERT_TRUE(ores) << ubench::string::perror(ores.error());

  auto sres = udp.send(packet);
  ASSERT_TRUE(sres) << ubench::string::perror(sres.error());
  ASSERT_EQ(*sres, 4);
}
