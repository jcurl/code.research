#ifndef SDD_CORE_LOCAL_COMPUTER_SERVICE_CONFIG_H
#define SDD_CORE_LOCAL_COMPUTER_SERVICE_CONFIG_H

#include <cstdint>
#include <string>

namespace sdd::core {

/// @brief Network settings for the local computer service.
struct local_computer_service_config {
  std::string multicast_group{"239.255.42.99"};  //< IPv4 multicast group.
  std::uint16_t port{52599};                     //< UDP port, bound to any.
};

}  // namespace sdd::core

#endif
