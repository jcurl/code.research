#pragma once

#include "sdd/computer.h"

namespace sdd {

class i_main_view {
 public:
  virtual ~i_main_view() = default;

  // May be called from any thread. The view marshals to its UI thread.
  virtual void show_computer(const computer& c) = 0;
};

}  // namespace sdd
