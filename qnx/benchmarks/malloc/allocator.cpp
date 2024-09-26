#include "allocator.h"

#include <array>
#include <cstddef>
#include <cstdint>
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
static std::array<std::uint8_t, localmemsize> localmem{};
static std::size_t offset{0};
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

}  // namespace

auto get_allocated_localmem() -> std::size_t { return offset; }

auto operator new(std::size_t size) noexcept(false) -> void * {
  std::size_t p = offset;
  offset += (size + 15) & ~0xF;
  if (offset > localmemsize) {
    std::cout << "Out of memory" << std::endl;
    std::abort();
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
  void *n = &localmem[p];
  return n;
}

auto operator new[](std::size_t size) noexcept(false) -> void * {
  std::size_t p = offset;
  offset += (size + 15) & ~0xF;
  if (offset > localmemsize) {
    std::cout << "Out of memory" << std::endl;
    std::abort();
  }

#ifndef NDEBUG
  std::size_t pb = p & ~0x3FFFFFF;
  std::size_t ob = offset & ~0x3FFFFFF;
  if (pb != ob) {
    std::cout << "Allocated: " << offset << " bytes of a total " << localmemsize
              << std::endl;
  }
#endif

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  void *n = &localmem[p];
  return n;
}

auto operator delete(void *) noexcept -> void {
  // Don't free anything.
}

auto operator delete(void *, std::size_t) noexcept -> void {
  // Don't free anything.
}

auto operator delete[](void *) noexcept -> void {
  // Don't free anything.
}

auto operator delete[](void *, std::size_t) noexcept -> void {
  // Don't free anything.
}
