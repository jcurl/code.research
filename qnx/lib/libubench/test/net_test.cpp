#include "ubench/net.h"

#include <arpa/inet.h>

#include <sstream>

#include <gtest/gtest.h>

TEST(ubench_net, is_multicast_false) {
  in_addr ipv4addr{};
  EXPECT_EQ(inet_pton(AF_INET, "192.168.1.1", &ipv4addr), 1);
  EXPECT_EQ(ubench::net::is_multicast(ipv4addr), false);

  EXPECT_EQ(inet_pton(AF_INET, "240.0.0.0", &ipv4addr), 1);
  EXPECT_EQ(ubench::net::is_multicast(ipv4addr), false);

  EXPECT_EQ(inet_pton(AF_INET, "223.255.255.255", &ipv4addr), 1);
  EXPECT_EQ(ubench::net::is_multicast(ipv4addr), false);

  EXPECT_EQ(inet_pton(AF_INET, "224.0.0.0", &ipv4addr), 1);
  EXPECT_EQ(ubench::net::is_multicast(ipv4addr), true);

  EXPECT_EQ(inet_pton(AF_INET, "239.255.255.255", &ipv4addr), 1);
  EXPECT_EQ(ubench::net::is_multicast(ipv4addr), true);
}

TEST(ubench_net, get_ether_multicast) {
  in_addr ipv4addr{};
  ubench::net::ether_addr ethaddr{};

  // Not a multicast address, should fail
  EXPECT_EQ(inet_pton(AF_INET, "192.168.1.1", &ipv4addr), 1);
  EXPECT_EQ(ubench::net::get_ether_multicast(ipv4addr, &ethaddr), false);

  // Is a multicast address.
  EXPECT_EQ(inet_pton(AF_INET, "224.0.0.252", &ipv4addr), 1);
  EXPECT_EQ(ubench::net::get_ether_multicast(ipv4addr, &ethaddr), true);
  EXPECT_EQ(ethaddr.ether_addr_octet[0], 0x01);
  EXPECT_EQ(ethaddr.ether_addr_octet[1], 0x00);
  EXPECT_EQ(ethaddr.ether_addr_octet[2], 0x5e);
  EXPECT_EQ(ethaddr.ether_addr_octet[3], 0x00);
  EXPECT_EQ(ethaddr.ether_addr_octet[4], 0x00);
  EXPECT_EQ(ethaddr.ether_addr_octet[5], 0xfc);
}

TEST(ubench_net, inet_ntos_sockaddr_in) {
  sockaddr_in ipv4addr{};
  std::string addr_s;
  ipv4addr.sin_family = AF_INET;

  ipv4addr.sin_port = 65535;
  EXPECT_EQ(inet_pton(AF_INET, "192.168.1.1", &ipv4addr.sin_addr), 1);
  addr_s = ubench::net::inet_ntos(ipv4addr);
  EXPECT_EQ(addr_s, std::string{"192.168.1.1:65535"});
  EXPECT_EQ(strcmp(addr_s.data(), "192.168.1.1:65535"), 0);
  EXPECT_EQ(strcmp(addr_s.c_str(), "192.168.1.1:65535"), 0);

  ipv4addr.sin_port = 0;
  EXPECT_EQ(inet_pton(AF_INET, "0.0.0.0", &ipv4addr.sin_addr), 1);
  addr_s = ubench::net::inet_ntos(ipv4addr);
  EXPECT_EQ(addr_s, std::string{"0.0.0.0:0"});
  EXPECT_EQ(strcmp(addr_s.data(), "0.0.0.0:0"), 0);
  EXPECT_EQ(strcmp(addr_s.c_str(), "0.0.0.0:0"), 0);

  ipv4addr.sin_port = 65535;
  EXPECT_EQ(inet_pton(AF_INET, "255.255.255.255", &ipv4addr.sin_addr), 1);
  addr_s = ubench::net::inet_ntos(ipv4addr);
  EXPECT_EQ(addr_s, std::string{"255.255.255.255:65535"});
  EXPECT_EQ(strcmp(addr_s.data(), "255.255.255.255:65535"), 0);
  EXPECT_EQ(strcmp(addr_s.c_str(), "255.255.255.255:65535"), 0);
}

TEST(ubench_net, inet_ntos_stream_sockaddr_in) {
  std::stringstream ss{};
  sockaddr_in ipv4addr{};
  ipv4addr.sin_family = AF_INET;

  ipv4addr.sin_port = 65535;
  EXPECT_EQ(inet_pton(AF_INET, "192.168.1.1", &ipv4addr.sin_addr), 1);
  ss << ipv4addr;
  EXPECT_EQ(ss.str(), std::string{"192.168.1.1:65535"});

  ss.str(std::string());
  ipv4addr.sin_port = 0;
  EXPECT_EQ(inet_pton(AF_INET, "0.0.0.0", &ipv4addr.sin_addr), 1);
  ss << ipv4addr;
  EXPECT_EQ(ss.str(), std::string{"0.0.0.0:0"});

  ss.str(std::string());
  ipv4addr.sin_port = 65535;
  EXPECT_EQ(inet_pton(AF_INET, "255.255.255.255", &ipv4addr.sin_addr), 1);
  ss << ipv4addr;
  EXPECT_EQ(ss.str(), std::string{"255.255.255.255:65535"});
}

TEST(ubench_net, inet_ntos_in_addr) {
  in_addr ipv4addr{};
  std::string addr_s;

  EXPECT_EQ(inet_pton(AF_INET, "192.168.1.1", &ipv4addr), 1);
  addr_s = ubench::net::inet_ntos(ipv4addr);
  EXPECT_EQ(addr_s, std::string{"192.168.1.1"});
  EXPECT_EQ(strcmp(addr_s.data(), "192.168.1.1"), 0);
  EXPECT_EQ(strcmp(addr_s.c_str(), "192.168.1.1"), 0);

  EXPECT_EQ(inet_pton(AF_INET, "0.0.0.0", &ipv4addr), 1);
  addr_s = ubench::net::inet_ntos(ipv4addr);
  EXPECT_EQ(addr_s, std::string{"0.0.0.0"});
  EXPECT_EQ(strcmp(addr_s.data(), "0.0.0.0"), 0);
  EXPECT_EQ(strcmp(addr_s.c_str(), "0.0.0.0"), 0);

  EXPECT_EQ(inet_pton(AF_INET, "255.255.255.255", &ipv4addr), 1);
  addr_s = ubench::net::inet_ntos(ipv4addr);
  EXPECT_EQ(addr_s, std::string{"255.255.255.255"});
  EXPECT_EQ(strcmp(addr_s.data(), "255.255.255.255"), 0);
  EXPECT_EQ(strcmp(addr_s.c_str(), "255.255.255.255"), 0);
}

TEST(ubench_net, inet_ntos_stream_in_addr) {
  std::stringstream ss{};
  in_addr ipv4addr{};

  EXPECT_EQ(inet_pton(AF_INET, "192.168.1.1", &ipv4addr), 1);
  ss << ipv4addr;
  EXPECT_EQ(ss.str(), std::string{"192.168.1.1"});

  ss.str(std::string());
  EXPECT_EQ(inet_pton(AF_INET, "0.0.0.0", &ipv4addr), 1);
  ss << ipv4addr;
  EXPECT_EQ(ss.str(), std::string{"0.0.0.0"});

  ss.str(std::string());
  EXPECT_EQ(inet_pton(AF_INET, "255.255.255.255", &ipv4addr), 1);
  ss << ipv4addr;
  EXPECT_EQ(ss.str(), std::string{"255.255.255.255"});
}

TEST(ubench_net, ether_ntos) {
  ubench::net::ether_addr addr{1, 0, 0x5e, 0, 0, 0xfc};

  std::string addr_s = ubench::net::ether_ntos(addr);
  EXPECT_EQ(addr_s, std::string{"01:00:5e:00:00:fc"});
  EXPECT_EQ(strcmp(addr_s.data(), "01:00:5e:00:00:fc"), 0);
  EXPECT_EQ(strcmp(addr_s.c_str(), "01:00:5e:00:00:fc"), 0);
}

TEST(ubench_net, ether_ntos_stream) {
  std::stringstream ss{};
  ubench::net::ether_addr addr{1, 0, 0x5e, 0, 0, 0xfc};

  ss << addr;
  EXPECT_EQ(ss.str(), std::string{"01:00:5e:00:00:fc"});
}

TEST(ubench_net, query_interfaces) { ubench::net::query_net_interfaces(); }
