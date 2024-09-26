#include "syspage.h"

#include <unistd.h>

auto get_syspage_size() -> int {
  return static_cast<int>(sysconf(_SC_PAGESIZE));
}
