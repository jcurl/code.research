#include "allocator.h"

#include <array>
#include <cstddef>
#include <iostream>
#include <new>

// We provide our own memory allocator routine, so that we avoid heap usage
// through the C++ runtime. We need to suppress all heap allocations, until
// we've called the `mallopt()` call to configure first.
//
// We can see the heap allocations with `valgrind --tool=massif malloc_bench`
// and analyse with `ms_print massif.out.XX > massif.out.txt`.

namespace {

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static std::array<std::byte, localmemsize> localmem{};
static std::size_t offset{0};
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

// The size of 'mem_block' rounded up, so adding this to the initial
// pointer maintains the alignment
auto next_align(std::size_t offset, std::size_t align) noexcept -> std::size_t {
  return (offset + align - 1) & (~align + 1);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto alloc(std::size_t size, std::size_t al) noexcept -> void * {
  std::size_t p = next_align(offset, al);
  offset = p + size;
  if (offset > localmemsize) {
    std::cout << "Out of memory" << std::endl;
    return nullptr;
  }

#ifndef NDEBUG
  std::size_t pb = p & ~0xFFFF;
  std::size_t ob = offset & ~0xFFFF;
  if (pb != ob) {
    std::cout << "Allocated: " << offset << " bytes of a total " << localmemsize
              << std::endl;
  }
#endif

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return &localmem[p];
}

}  // namespace

auto get_allocated_localmem() -> std::size_t { return offset; }

auto operator new(std::size_t size, const std::nothrow_t &) noexcept -> void * {
  return alloc(size, sizeof(std::max_align_t));
}

auto operator new(std::size_t size) -> void * {
  auto p = alloc(size, sizeof(std::max_align_t));
  if (!p) throw std::bad_alloc{};
  return p;
}

auto operator delete(void *, const std::nothrow_t &) noexcept -> void {
  // Don't free anything.
}

auto operator delete(void *) noexcept -> void {
  // Don't free anything.
}

auto operator delete(void *, std::size_t) noexcept -> void {
  // Don't free anything.
}

auto operator new[](std::size_t size, const std::nothrow_t &) noexcept
    -> void * {
  return alloc(size, sizeof(std::max_align_t));
}

auto operator new[](std::size_t size) -> void * {
  auto p = alloc(size, sizeof(std::max_align_t));
  if (!p) throw std::bad_alloc{};
  return p;
}

auto operator delete[](void *, const std::nothrow_t &) noexcept -> void {
  // Don't free anything.
}

auto operator delete[](void *) noexcept -> void {
  // Don't free anything.
}

auto operator delete[](void *, std::size_t) noexcept -> void {
  // Don't free anything.
}

auto operator new(std::size_t size, std::align_val_t al,
    const std::nothrow_t &) noexcept -> void * {
  return alloc(size, static_cast<std::size_t>(al));
}

auto operator new(std::size_t size, std::align_val_t al) -> void * {
  auto p = alloc(size, static_cast<std::size_t>(al));
  if (!p) throw std::bad_alloc{};
  return p;
}

auto operator delete(void *, std::align_val_t, const std::nothrow_t &) noexcept
    -> void {
  // Don't free anything.
}

auto operator delete(void *, std::align_val_t) noexcept -> void {
  // Don't free anything.
}

auto operator delete(void *, std::size_t, std::align_val_t) noexcept -> void {
  // Don't free anything.
}

auto operator new[](std::size_t size, std::align_val_t al,
    const std::nothrow_t &) noexcept -> void * {
  return alloc(size, static_cast<std::size_t>(al));
}

auto operator new[](std::size_t size, std::align_val_t al) -> void * {
  auto p = alloc(size, static_cast<std::size_t>(al));
  if (!p) throw std::bad_alloc{};
  return p;
}

auto operator delete[](
    void *, std::align_val_t, const std::nothrow_t &) noexcept -> void {
  // Don't free anything.
}

auto operator delete[](void *, std::align_val_t) noexcept -> void {
  // Don't free anything.
}

auto operator delete[](void *, std::size_t, std::align_val_t) noexcept -> void {
  // Don't free anything.
}
