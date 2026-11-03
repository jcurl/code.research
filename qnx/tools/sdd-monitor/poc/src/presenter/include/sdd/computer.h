#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sdd {

using computer_id = std::uint64_t;

struct interface_info {
  std::string name;
  std::string mac;
  std::vector<std::string> ips;
};

// A partial record (only id and name) means "discovered, details not yet
// queried".
struct computer {
  computer_id id{};
  std::string name;
  std::string os;
  std::vector<interface_info> interfaces;
};

}  // namespace sdd
