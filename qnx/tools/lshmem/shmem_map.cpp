#include "shmem_map.h"

// There are six ways two memory regions can overlap, given in the diagram
// below. The compare_and_add() function checks for overlap and adds to the
// existing vector the amount of overlapping memory.

// clang-format off
//
// #1          | #2          | #3          | #4          | #5          | #6
//  +--+       |  +--+ +--+  |  +--+ +--+  |  +--+       |  +--+       |  +--+
//  |  |       |  |  | |  |  |  |  | +--+  |  |  |       |  |  | +--+  |  |  |
//  |  |       |  |  | |  |  |  |  |       |  |  | +--+  |  |  | +--+  |  |  | +--+
//  +--+       |  +--+ +--+  |  +--+       |  +--+ +--+  |  +--+       |  +--+ |  |
//             |             |             |             |             |       |  |
//       +--+  |             |             |             |             |       +--+
//       |  |  |             |             |             |             |
//       |  |  |             |             |             |             |
//       +--+  |             |             |             |             |
//
// clang-format on

namespace {

auto inline is_same(const map_line& l1, const map_line& l2) -> bool {
  return l1.ri_flags == l2.ri_flags && l1.ri_prot == l2.ri_prot &&
         l1.dev == l2.dev && l1.ino == l2.ino && l1.object == l2.object;
}

}  // namespace

auto shmem_map::compare_and_add(const map_line& p1, const map_line& p2)
    -> void {
  auto ostart = std::max(p1.phys_addr, p2.phys_addr);
  auto oend = std::min(p1.phys_addr + p1.phys_len, p2.phys_addr + p2.phys_len);

  // There is no overlap if it starts afer the end.
  if (ostart >= oend) return;

  auto olength = oend - ostart;
  for (auto& entry : overlap_) {
    if (is_same(entry.p1, p1) && is_same(entry.p2, p2)) {
      entry.size += olength;
      return;
    }
  }
  auto& new_entry = overlap_.emplace_back(shmem_entry{});
  new_entry.p1.ri_flags = p1.ri_flags;
  new_entry.p1.ri_prot = p1.ri_prot;
  new_entry.p1.dev = p1.dev;
  new_entry.p1.ino = p1.ino;
  new_entry.p1.object = p1.object;
  new_entry.p2.ri_flags = p2.ri_flags;
  new_entry.p2.ri_prot = p2.ri_prot;
  new_entry.p2.dev = p2.dev;
  new_entry.p2.ino = p2.ino;
  new_entry.p2.object = p2.object;
  new_entry.size = olength;
}
