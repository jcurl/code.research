#ifndef BENCHMARK_LSHMEM_PID_MAPPING_H
#define BENCHMARK_LSHMEM_PID_MAPPING_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <list>
#include <memory>
#include <string_view>

#include "stdext/expected.h"
#include "ubench/str_intern.h"

/// @brief describes a contiguous physical memory region.
struct map_line {
  std::uintptr_t phys_addr;  //< Physical address start.
  std::size_t phys_len;      //< Physical address length.
  int ri_flags;              //< Memory allocation flags.
  int ri_prot;               //< Memory allocation protection.
  dev_t dev;                 //< Associated device.
  ino_t ino;                 //< Associated inode.
  std::string_view object;   //< Pointer to name.
};

/// @brief List of contiguous physical memory regions.
class pid_mapping final {
 public:
  pid_mapping(std::shared_ptr<ubench::string::str_intern> strings)
      : strings_{std::move(strings)} {}
  pid_mapping(const pid_mapping& other) = default;
  auto operator=(const pid_mapping& other) -> pid_mapping& = default;
  pid_mapping(pid_mapping&&) noexcept = default;
  auto operator=(pid_mapping&& other) noexcept -> pid_mapping& = default;
  ~pid_mapping() = default;

  /// @brief Adds the given map_line into the structure, merging as necessary.
  ///
  /// @param map_line The map_line to merge.
  auto insert(map_line&& line) -> void;

  /// @brief Get the memory mapping ranges.
  ///
  /// @return a list of memory mappings.
  [[nodiscard]] auto map() const -> const std::list<map_line>& { return map_; }

 private:
  std::list<map_line> map_{};
  std::shared_ptr<ubench::string::str_intern> strings_{};
};

/// @brief Load a mappings file.
///
/// @param mapping path to the mappings file.
///
/// @param read check also read-only mappings.
///
/// @return the map, or the error code.
auto load_mapping(std::filesystem::path mapping, bool read,
    std::shared_ptr<ubench::string::str_intern> strings)
    -> stdext::expected<pid_mapping, int>;

#endif
