#ifndef BENCHMARK_LSHMEM_SHMEM_MAP_H
#define BENCHMARK_LSHMEM_SHMEM_MAP_H

#include <vector>

#include "pid_mapping.h"

/// @brief An overlapped shared memory entry between two PIDs.
///
/// The phys_addr and phys_len have no meaning and are set to zero.
struct shmem_entry {
  map_line p1;
  map_line p2;
  std::size_t size{0};
};

class shmem_map {
 public:
  shmem_map() = default;
  shmem_map(const shmem_map&) = default;
  auto operator=(const shmem_map&) -> shmem_map& = default;
  shmem_map(shmem_map&&) noexcept = default;
  auto operator=(shmem_map&&) noexcept -> shmem_map& = default;
  virtual ~shmem_map() = default;

  auto compare_and_add(const map_line& p1, const map_line& p2) -> void;

  [[nodiscard]] auto overlap() const -> const std::vector<shmem_entry>& {
    return overlap_;
  }

 private:
  std::vector<shmem_entry> overlap_{};
};

#endif
