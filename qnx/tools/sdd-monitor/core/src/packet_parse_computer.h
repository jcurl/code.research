#ifndef SDD_CORE_PACKET_PARSE_COMPUTER_H
#define SDD_CORE_PACKET_PARSE_COMPUTER_H

#include <algorithm>
#include <optional>
#include <string_view>
#include <vector>

#include "sdd/computer.h"
#include "sjson/json_reader.h"

namespace sdd::core {

/// @brief Find the known computer for target.
///
/// If target.id is not -1, it is the index into known. Otherwise the computer
/// with the same host and domain is searched for.
///
/// @param known The computers to search.
/// @param target The computer to find, by id or by host and domain.
/// @return An iterator to the computer, or end() if it is not known.
inline auto find_computer(std::vector<computer>& known,
    const computer& target) noexcept -> std::vector<computer>::iterator {
  if (target.id != -1) {
    if (target.id < 0 || static_cast<std::size_t>(target.id) >= known.size()) {
      return known.end();
    }
    return known.begin() + target.id;
  }
  return std::find_if(known.begin(), known.end(), [&target](const computer& c) {
    return c.hostname == target.hostname && c.domain == target.domain;
  });
}

/// @brief Parses a computer discovery packet. Use once per packet.
class packet_parse_computer {
 public:
  /// @param json The packet. Must remain valid until parse() returns.
  explicit packet_parse_computer(std::string_view json);

  packet_parse_computer(const packet_parse_computer&) = delete;
  auto operator=(const packet_parse_computer&)
      -> packet_parse_computer& = delete;
  packet_parse_computer(packet_parse_computer&&) = delete;
  auto operator=(packet_parse_computer&&) -> packet_parse_computer& = delete;
  ~packet_parse_computer() = default;

  /// @brief Parse the input and match against known computers.
  /// @param known The computers already known. A known host and domain skips
  /// parsing of its interfaces.
  /// @return The parsed computer, or std::nullopt if the packet is invalid.
  auto parse(std::vector<computer>& known) -> std::optional<computer>;

 private:
  template <typename Container, typename Visit>
  auto for_each(const Container& container, Visit visit) -> void;
  auto parse_interface(
      const ubench::sjson::json_object& fields, interface& intf) -> void;

  ubench::sjson::json_reader reader_;
  computer result_{};
  bool valid_{true};
};

}  // namespace sdd::core

#endif
