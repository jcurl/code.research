#include "allocator.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <new>

namespace {

// Maintain the memory allocation metrics.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
mem_metrics metrics_{};

struct mem_block {
  int block;
};

constexpr ptrdiff_t OFFSET = std::max<ptrdiff_t>(16, sizeof(mem_block));

auto alloc(mem_metrics &metrics, int size) -> void {
  ++metrics.allocs;
  metrics.total_alloc += size;
  metrics.current_alloc += size;
  if (metrics.current_alloc > metrics.max_alloc)
    metrics.max_alloc = metrics.current_alloc;
}

auto dealloc(mem_metrics &metrics, int size) -> void {
  ++metrics.frees;
  metrics.total_free += size;
  metrics.current_alloc -= size;
}

auto reset(mem_metrics &metrics) -> void {
  metrics = {
      0,
  };
}

[[nodiscard]] inline auto alloc(std::size_t size) -> void * {
  assert(size <= std::numeric_limits<int>::max());
  alloc(metrics_, static_cast<int>(size));

  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
  void *p = std::malloc(size + OFFSET);
  if (p == nullptr) throw std::bad_alloc();
  static_cast<mem_block *>(p)->block = static_cast<int>(size);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  return static_cast<char *>(p) + OFFSET;
}

inline auto dealloc(void *p) -> void {
  // NOLINTNEXTLINE(ycppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-reinterpret-cast)
  auto m = reinterpret_cast<mem_block *>(static_cast<char *>(p) - OFFSET);

  // We know that m->block is a positive value set by alloc().
  dealloc(metrics_, static_cast<int>(m->block));

  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
  std::free(m);
}

}  // namespace

auto reset_alloc() -> void { reset(metrics_); }

auto get_stats() -> mem_metrics { return metrics_; }

// Assume no memory operators are replaced, and only done here. Then
// `new[](size)` calls `new(size)` and `new[](size, al)` calls `new(size, al)`.

auto operator new(std::size_t size) -> void * { return alloc(size); }

auto operator new([[maybe_unused]] std::size_t count,
    [[maybe_unused]] std::align_val_t al) -> void * {
  std::abort();
}

auto operator delete(void *p) noexcept -> void { dealloc(p); }

auto operator delete(void *p, std::align_val_t) noexcept -> void { dealloc(p); }

auto operator delete(void *p, std::size_t) noexcept -> void { dealloc(p); }
