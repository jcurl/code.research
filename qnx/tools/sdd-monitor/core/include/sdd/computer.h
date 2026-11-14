#ifndef SDD_CORE_COMPUTER_H
#define SDD_CORE_COMPUTER_H

#include <cstdint>
#include <ctime>
#include <map>
#include <string>
#include <vector>

namespace sdd::core {

/// @brief Describes a network interface.
struct interface {
  std::string mac;                //< The interface MAC address.
  std::uint32_t mtu;              //< The interface MTU.
  std::vector<std::string> ipv4;  //< List of IPv4 addresses and netmasks.
};

/// @brief Describes a computer notification.
struct computer {
  int id;                //< Identifier for this computer instance.
  time_t last_seen;      //< Unix timestamp when last update received.
  std::string hostname;  //< Hostname.
  std::string domain;    //< Domain name.
  std::map<std::string, interface> interfaces;  //< List of interfaces.
};

}  // namespace sdd::core

#endif
