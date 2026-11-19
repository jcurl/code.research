#include "packet_parse_computer.h"

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace sdd::core {

namespace {

using ubench::sjson::json_array;
using ubench::sjson::json_int;
using ubench::sjson::json_object;
using ubench::sjson::json_string;

}  // namespace

packet_parse_computer::packet_parse_computer(std::string_view json)
    : reader_{json} {}

/// @brief Visit each member or element. A visit that returns false marks the
/// packet invalid and closes the reader, so the remaining iterators see no
/// more data.
/// @tparam Container The type of the JSON container being iterated.
/// @tparam Visit The callable taking each item and returning true to continue.
/// @param container The container to iterate over.
/// @param visit The callable to operate on each item.
template <typename Container, typename Visit>
auto packet_parse_computer::for_each(const Container& container, Visit visit)
    -> void {
  for (const auto& item : container) {
    if (!visit(item)) {
      valid_ = false;
      reader_.close();
    }
  }
}

/// @brief Store the interface fields. A field of the wrong type fails the
/// packet, except for the optional mtu.
/// @param fields The JSON object holding the interface fields.
/// @param intf The interface to store the fields in.
auto packet_parse_computer::parse_interface(
    const json_object& fields, data::interface& intf) -> void {
  for_each(fields, [this, &intf](const auto& field) -> bool {
    const auto& value = field.value();
    if (field.key() == "mac") {
      const auto* mac = std::get_if<json_string>(&value);
      if (mac == nullptr) return false;
      intf.mac = std::string_view{*mac};
    } else if (field.key() == "mtu") {
      // Optional: a mistyped MTU stays zero rather than failing the packet.
      if (const auto* mtu = std::get_if<json_int>(&value))
        intf.mtu = static_cast<std::uint32_t>(*mtu);
    } else if (field.key() == "ipv4") {
      const auto* addresses = std::get_if<json_array>(&value);
      if (addresses == nullptr) return false;
      for_each(*addresses, [&intf](const auto& address) -> bool {
        const auto* ip = std::get_if<json_string>(&address);
        if (ip == nullptr) return false;
        intf.ipv4.emplace_back(std::string_view{*ip});
        return true;
      });
      return valid_;
    }
    return true;
  });
}

auto packet_parse_computer::parse(std::vector<data::computer>& known)
    -> std::optional<data::computer> {
  result_.id = -1;
  result_.last_seen = std::time(nullptr);

  const auto root = reader_.root();
  const auto* members = root ? std::get_if<json_object>(&*root) : nullptr;
  if (members == nullptr) return std::nullopt;

  for_each(*members, [this, &known](const auto& member) -> bool {
    const auto& value = member.value();
    if (member.key() == "host") {
      const auto* host = std::get_if<json_string>(&value);
      if (host == nullptr) return false;
      result_.hostname = std::string_view{*host};
    } else if (member.key() == "domain") {
      const auto* domain = std::get_if<json_string>(&value);
      if (domain == nullptr) return false;
      result_.domain = std::string_view{*domain};
    } else if (member.key() == "interfaces") {
      const auto* interfaces = std::get_if<json_object>(&value);
      if (interfaces == nullptr) return false;

      // A known computer keeps its interfaces; nothing more needs reading.
      const auto existing = find_computer(known, result_);
      if (existing != known.end()) {
        data::computer refreshed = *existing;
        refreshed.id = -1;
        refreshed.last_seen = result_.last_seen;
        result_ = std::move(refreshed);
        reader_.close();
        return true;
      }

      for_each(*interfaces, [this](const auto& entry) -> bool {
        const auto* fields = std::get_if<json_object>(&entry.value());
        if (fields == nullptr) return false;
        parse_interface(*fields, result_.interfaces[std::string{entry.key()}]);
        return valid_;
      });
    }
    return true;
  });

  if (!valid_ || reader_.has_error()) return std::nullopt;
  return std::move(result_);
}

}  // namespace sdd::core
