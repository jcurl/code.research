#ifndef BENCHMARK_LSHMEM_PID_MAPPING_H
#define BENCHMARK_LSHMEM_PID_MAPPING_H

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <filesystem>
#include <forward_list>
#include <list>
#include <string_view>

#include "stdext/expected.h"

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
  pid_mapping() = default;

  /// @brief Copy constructor.
  ///
  /// Copying is slow as the intern strings all need to be updated.
  ///
  /// @param other the other object to copy.
  pid_mapping(const pid_mapping& other)
      : map_{other.map_}, strings_{other.strings_} {
    fix_intern();
  }

  /// @brief Copy assignment operator.
  ///
  /// Copying is slow as the intern strings all need to be updated.
  ///
  /// @param other the other object to copy.
  ///
  /// @return a reference to this.
  auto operator=(const pid_mapping& other) -> pid_mapping& {
    if (this == &other) return *this;
    map_ = other.map_;
    strings_ = other.strings_;
    last_ = {};  // Cache only, not valid on copy.
    fix_intern();
    return *this;
  }
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
  const std::string* last_{};
  std::forward_list<std::string> strings_{};

  /// @brief Given a string view, return an interned string.
  ///
  /// @param str The string_view that should be interned.
  ///
  /// @return A reference to a string that is interned.
  auto string_intern(std::string_view str) -> const std::string&;

  /// @brief Fix all copies of strings in map_ to point to the corect intern.
  ///
  /// When copying this object, the map_ will have strings pointing to the old
  /// copy that aren't updated (because the references aren't copied, they're
  /// assumed to be valid). All the strings in the intern were moved though, so
  /// just need to update them all.
  auto fix_intern() -> void;
};

/// @brief Load a mappings file.
///
/// @param mapping path to the mappings file.
///
/// @param read check also read-only mappings.
///
/// @return the map, or the error code.
auto load_mapping(std::filesystem::path mapping, bool read)
    -> stdext::expected<pid_mapping, int>;

#endif
