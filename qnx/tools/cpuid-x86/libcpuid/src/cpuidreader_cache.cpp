#include "cpuid/cpuidreader_cache.h"

#include <cerrno>
#include <optional>

#include "cpuid/cpuid.h"
#include "stdext/expected.h"

namespace rjcp::cpuid {

auto cpuidreader_cache::has_cpuid() -> bool {
  if (reader_) return reader_->has_cpuid();

  // There must be at least one core present.
  if (cache_.empty()) return false;

  // And there must be at least one value present.
  for (const auto& [core, cpuid_cache] : cache_) {
    if (cpuid_cache.has_cpuid()) return true;
  }
  return false;
}

auto cpuidreader_cache::add_cpuid(
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    unsigned int core, cpuidreg eax, cpuidreg ecx, cpuid_res& res) -> bool {
  if (cores() <= core) return false;

  // The core isn't cached.
  cpuid_req req{eax, ecx};
  cpuid_info info{req, res};

  auto cpuid_map_it = cache_.find(core);
  if (cpuid_map_it == cache_.end()) {
    // The core has no entry. Updating the cache is always true.
    cpuid_cache cpuid{};
    cpuid.update_cache(info);

    // success is always true, unless there was a race condition. This results
    // in undefined behaviour, but at least let the user know.
    auto [it, success] = cache_.emplace(core, std::move(cpuid));
    return success;
  } else {
    // The core is cached, but there is no register.
    auto& cpuid = cpuid_map_it->second;
    cpuid.update_cache(info);
    return true;
  }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto cpuidreader_cache::get_cpuid(unsigned int core, cpuidreg eax, cpuidreg ecx)
    -> std::optional<cpuid_res> {
  auto cpuid_map_it = cache_.find(core);
  if (cpuid_map_it != cache_.end()) {
    auto& cpuid = cpuid_map_it->second;
    return cpuid.cpuid(eax, ecx);
  }
  return std::nullopt;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto cpuidreader_cache::cpuid(cpuidreg eax, cpuidreg ecx)
    -> std::optional<cpuid_res> {
  // Check if the context is valid, if so then get the core, else free it for
  // good.
  unsigned int core{};
  if (ctx_) {
    if (ctx_.use_count() > 1) {
      core = ctx_->core();
    } else {
      ctx_ = nullptr;
    }
  }

  auto cached_res = get_cpuid(core, eax, ecx);
  if (cached_res) return cached_res;

  if (!reader_) return std::nullopt;
  if (reader_->cores() <= core) return std::nullopt;

  auto ctx = reader_->enable_core(core);
  if (!ctx) return std::nullopt;

  auto res = reader_->cpuid(eax, ecx);
  if (!res) return std::nullopt;

  if (!add_cpuid(core, eax, ecx, *res)) return std::nullopt;
  return res;
}

auto cpuidreader_cache::enable_core(unsigned int core)
    -> stdext::expected<std::unique_ptr<cpuid_ctx>, int> {
  if (core >= cores()) return stdext::unexpected{EINVAL};
  if (ctx_ && ctx_.use_count() > 1) {
    return stdext::unexpected{EINVAL};
  }

  ctx_ = std::make_shared<core_ctx>(core);
  return std::make_unique<detail::cpuid_basic_ctx<core_ctx>>(ctx_);
}

}  // namespace rjcp::cpuid
