#pragma once

#include <optional>

#include "sdd/computer.h"

namespace sdd {

class i_view_listener {
 public:
  virtual ~i_view_listener() = default;

  // Called on the UI thread. std::nullopt means nothing is selected.
  virtual void on_selected(std::optional<computer_id> id) = 0;
};

}  // namespace sdd
