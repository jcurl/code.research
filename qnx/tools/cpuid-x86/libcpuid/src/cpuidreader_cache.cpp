#include "cpuid/cpuidreader_cache.h"

#include <cerrno>
#include <optional>

#include "cpuid/cpuid.h"
#include "stdext/expected.h"

namespace rjcp::cpuid {

auto cpuidreader_cache::has_cpuid() -> bool {
  if (reader_) return reader_->has_cpuid();

  // There must be at least one core and one value present.
  return (!cache_.empty());
}

auto cpuidreader_cache::add_cpuid(
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    unsigned int core, cpuidreg eax, cpuidreg ecx, cpuid_res& res) -> bool {
  if (cores() <= core) return false;

  // The core isn't cached.
  cpuid_req req{eax, ecx};
  auto cpuid_map_it = cache_.find(core);
  if (cpuid_map_it == cache_.end()) {
    decltype(cache_)::mapped_type cpuid{};
    auto [it, success] = cpuid.emplace(req, cpuid_info{req, res});
    if (!success) return false;

    auto [it2, success2] = cache_.emplace(core, cpuid);
    if (!success2) return false;
  } else {
    // The core is cached, but there is no register.
    auto cpuid = cpuid_map_it->second;
    auto [it, success] = cpuid.emplace(req, cpuid_info{req, res});
    if (!success) return false;
  }
  return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto cpuidreader_cache::get_cpuid(unsigned int core, cpuidreg eax, cpuidreg ecx)
    -> std::optional<cpuid_res> {
  cpuid_req req{eax, ecx};
  auto cpuid_map_it = cache_.find(core);
  if (cpuid_map_it != cache_.end()) {
    auto cpuid = cpuid_map_it->second;
    auto cpuid_info_it = cpuid.find(req);
    if (cpuid_info_it != cpuid.end()) {
      return cpuid_info_it->second.res;
    }
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
