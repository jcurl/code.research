#include <cerrno>

#include "stdext/expected.h"
#include "ubench/os.h"

namespace ubench::os {

auto get_syspage_size() -> stdext::expected<unsigned int, int> {
  return stdext::unexpected{ENOSYS};
}

}  // namespace ubench::os
