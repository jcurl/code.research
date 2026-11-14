#include "sdd/local_computer_service.h"

#include <algorithm>
#include <utility>

namespace sdd::core {

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
