#include <Windows.h>

#include "ubench/os.h"

namespace ubench::os {

auto get_syspage_size() -> stdext::expected<unsigned int, int> {
  SYSTEM_INFO info{};
  GetSystemInfo(&info);
  return info.dwPageSize;
}

}  // namespace ubench::os
