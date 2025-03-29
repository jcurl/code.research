#include "allocator.h"

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>

namespace {

// Maintain the memory allocation metrics.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
mem_metrics metrics_{};

struct mem_block {
  mem_block(std::size_t size) : block{size} {}

  std::size_t block;
};

auto alloc(mem_metrics &metrics, std::size_t size) -> void {
  ++metrics.allocs;
  metrics.total_alloc += size;
  metrics.current_alloc += static_cast<std::int64_t>(size);
  if (metrics.current_alloc > 0 &&
      static_cast<std::uint64_t>(metrics.current_alloc) > metrics.max_alloc)
    metrics.max_alloc = metrics.current_alloc;
}

auto dealloc(mem_metrics &metrics, std::size_t size) -> void {
  ++metrics.frees;
  metrics.total_free += size;
  metrics.current_alloc -= static_cast<std::int64_t>(size);
}

auto reset(mem_metrics &metrics) -> void {
  metrics = {
      0,
  };
}

}  // namespace

auto reset_alloc() -> void { reset(metrics_); }

auto get_stats() -> mem_metrics { return metrics_; }

// Assume no memory operators are replaced, and only done here. Then
// `new[](size)` calls `new(size)` and `new[](size, al)` calls `new(size, al)`.

// The size of 'mem_block' rounded up, so adding this to the initial
// pointer maintains the alignment
template <class T>
constexpr auto sizeof_rounded(std::size_t align) noexcept -> std::size_t {
  return (sizeof(T) + align - 1) & (~align + 1);
}

auto operator new(std::size_t size, const std::nothrow_t &) noexcept -> void * {
  std::size_t msize =
      sizeof_rounded<mem_block>(sizeof(std::max_align_t)) + size;

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
  auto p = std::malloc(msize);
  if (!p) return nullptr;

  // Allocate our accounting structure directly in place.
  new (p) mem_block{size};
  alloc(metrics_, size);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  return static_cast<std::byte *>(p) +
         sizeof_rounded<mem_block>(sizeof(std::max_align_t));
}

auto operator new(std::size_t size) -> void * {
  auto p = ::operator new(size, std::nothrow);
  if (!p) throw std::bad_alloc{};
  return p;
}

auto operator delete(void *p, const std::nothrow_t &) noexcept -> void {
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
  auto rp = static_cast<void *>(
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      static_cast<std::byte *>(p) -
      sizeof_rounded<mem_block>(sizeof(std::max_align_t)));
  auto mb = static_cast<mem_block *>(rp);
  dealloc(metrics_, mb->block);
  mb->~mem_block();

  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
  std::free(rp);
}

auto operator delete(void *p) noexcept -> void {
  ::operator delete(p, std::nothrow);
}

auto operator delete(void *p, std::size_t) noexcept -> void {
  ::operator delete(p, std::nothrow);
}

auto operator new[](std::size_t size, const std::nothrow_t &) noexcept
    -> void * {
  std::size_t msize =
      sizeof_rounded<mem_block>(sizeof(std::max_align_t)) + size;

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
  auto p = std::malloc(msize);
  if (!p) return nullptr;

  // Allocate our accounting structure directly in place.
  new (p) mem_block{size};
  alloc(metrics_, size);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  return static_cast<std::byte *>(p) +
         sizeof_rounded<mem_block>(sizeof(std::max_align_t));
}

auto operator new[](std::size_t size) -> void * {
  auto p = ::operator new[](size, std::nothrow);
  if (!p) throw std::bad_alloc{};
  return p;
}

auto operator delete[](void *p, const std::nothrow_t &) noexcept -> void {
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
  auto rp = static_cast<void *>(
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      static_cast<std::byte *>(p) -
      sizeof_rounded<mem_block>(sizeof(std::max_align_t)));
  auto mb = static_cast<mem_block *>(rp);
  dealloc(metrics_, mb->block);
  mb->~mem_block();

  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
  std::free(rp);
}

auto operator delete[](void *p) noexcept -> void {
  ::operator delete[](p, std::nothrow);
}

auto operator delete[](void *p, std::size_t) noexcept -> void {
  ::operator delete[](p, std::nothrow);
}

auto operator new(std::size_t size, std::align_val_t al,
    const std::nothrow_t &) noexcept -> void * {
  std::size_t msize =
      sizeof_rounded<mem_block>(static_cast<std::size_t>(al)) + size;

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
  auto p = std::aligned_alloc(static_cast<std::size_t>(al), msize);
  if (!p) return nullptr;

  // Allocate our accounting structure directly in place.
  new (p) mem_block{size};
  alloc(metrics_, size);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  return static_cast<std::byte *>(p) +
         sizeof_rounded<mem_block>(static_cast<std::size_t>(al));
}

auto operator new(std::size_t size, std::align_val_t al) -> void * {
  auto p = ::operator new(size, al, std::nothrow);
  if (!p) throw std::bad_alloc{};
  return p;
}

auto operator delete(
    void *p, std::align_val_t al, const std::nothrow_t &) noexcept -> void {
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
  auto rp = static_cast<void *>(
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      static_cast<std::byte *>(p) -
      sizeof_rounded<mem_block>(static_cast<std::size_t>(al)));
  auto mb = static_cast<mem_block *>(rp);
  dealloc(metrics_, mb->block);
  mb->~mem_block();

  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
  std::free(rp);
}

auto operator delete(void *p, std::align_val_t al) noexcept -> void {
  ::operator delete(p, al, std::nothrow);
}

auto operator delete(void *p, std::size_t, std::align_val_t al) noexcept
    -> void {
  ::operator delete(p, al, std::nothrow);
}

auto operator new[](std::size_t size, std::align_val_t al,
    const std::nothrow_t &) noexcept -> void * {
  std::size_t msize =
      sizeof_rounded<mem_block>(static_cast<std::size_t>(al)) + size;

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
  auto p = std::aligned_alloc(static_cast<std::size_t>(al), msize);
  if (!p) return nullptr;

  // Allocate our accounting structure directly in place.
  new (p) mem_block{size};
  alloc(metrics_, size);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  return static_cast<std::byte *>(p) +
         sizeof_rounded<mem_block>(static_cast<std::size_t>(al));
}

auto operator new[](std::size_t size, std::align_val_t al) -> void * {
  auto p = ::operator new[](size, al, std::nothrow);
  if (!p) throw std::bad_alloc{};
  return p;
}

auto operator delete[](
    void *p, std::align_val_t al, const std::nothrow_t &) noexcept -> void {
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
  auto rp = static_cast<void *>(
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      static_cast<std::byte *>(p) -
      sizeof_rounded<mem_block>(static_cast<std::size_t>(al)));
  auto mb = static_cast<mem_block *>(rp);
  dealloc(metrics_, mb->block);
  mb->~mem_block();

  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
  std::free(rp);
}

auto operator delete[](void *p, std::align_val_t al) noexcept -> void {
  ::operator delete[](p, al, std::nothrow);
}

auto operator delete[](void *p, std::size_t, std::align_val_t al) noexcept
    -> void {
  ::operator delete[](p, al, std::nothrow);
}
