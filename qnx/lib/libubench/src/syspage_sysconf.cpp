#include <unistd.h>

#include "ubench/os.h"

namespace ubench::os {

auto get_syspage_size() -> stdext::expected<unsigned int, int> {
  // Cygwin returns an incorrect answer - 65536.
  return static_cast<unsigned int>(sysconf(_SC_PAGESIZE));
}

}  // namespace ubench::os
