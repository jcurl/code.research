#include "cpuid/cpuid_cache.h"

#include "cpuid/cpuid.h"

namespace rjcp::cpuid {

// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
cpuid_cache::cpuid_cache(const std::vector<cpuid_info>& info) {
  // Move the data received into our unordered map (we do a partial move).
  // While we actually do a copy, the contract implies that the user is
  // expected to not use the object afterwards.
  map_.reserve(info.size());
  for (const auto& item : info) {
    map_.emplace(item.req, item);
  }
}

[[nodiscard]] auto cpuid_cache::has_cpuid() const -> bool {
  return !map_.empty();
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto cpuid_cache::cpuid(cpuidreg eax, cpuidreg ecx)
    -> std::optional<cpuid_res> {
  if (!has_cpuid()) return std::nullopt;

  cpuid_req req{eax, ecx};
  auto cpuid_info = map_.find(req);
  if (cpuid_info == map_.end()) {
    return std::nullopt;
  }
  return cpuid_info->second.res;
}

auto cpuid_cache::update_cache(const cpuid_info& info) -> bool {
  auto [it, result] = map_.emplace(info.req, info);
  if (result) return true;

  map_[info.req] = info;
  return false;
}

}  // namespace rjcp::cpuid
