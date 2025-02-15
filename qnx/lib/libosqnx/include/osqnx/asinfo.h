#ifndef LIB_OSQNX_ASINFO_H
#define LIB_OSQNX_ASINFO_H

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace os::qnx {

/// @brief Describes an entry in QNX's sysinfo asinfo section.
struct asinfo_entry {
  uintptr_t start;        //< The start physical address.
  uintptr_t end;          //< The end physical address (inclusive).
  std::string_view name;  //< The NUL-terminated name. Taken from the syspage.

  /// @brief Get the size of this asinfo entry.
  ///
  /// @returns the size, in bytes, of this asinfo entry. Zero if empty.
  [[nodiscard]] auto size() const -> std::size_t {
    if (end < start) {
      return 0;
    } else {
      return end - start + 1;
    }
  }

  /// @brief Checks if a pointer value is in range of this entry.
  ///
  /// @returns true if in range, false otherwise.
  [[nodiscard]] auto in_range(uintptr_t offset) const -> bool {
    return start <= end && offset >= start && offset <= end;
  }
};

/// @brief The type for all asinfo entries.
using asinfo_map = std::vector<asinfo_entry>;

/// @brief Read the asinfo page.
///
/// Returns all entries in the asinfo page from QNX that have no children. The
/// start and the end define the physical address ranges. The name is
/// NUL-terminated.
///
/// @return A vector of memory entries. Addresses are sorted by the start
/// address.
auto get_asinfo() -> const asinfo_map&;

/// @brief Get the name of the typed memory object containing the pointer
///
/// @param start the address to get the mapping for.
///
/// @returns the name of the typed memory, or nothing if not present.
auto get_tymem_name(uintptr_t start) -> std::optional<std::string_view>;

/// @brief Get the total amount of sysram in the system.
///
/// @returns the total amount of sysram in the system.
auto get_sysram() -> std::size_t;

}  // namespace os::qnx

#endif