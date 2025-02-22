#ifndef UBENCH_OS_H
#define UBENCH_OS_H

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <string>

#include "stdext/expected.h"

namespace ubench::os {

/// @brief Provide a wrapper around an OS struct of variable size.
///
/// It's common practice in system headers to define structures whose last field
/// is a single element, but in reality it is of variable size. The user is
/// expected to allocate a buffer sufficiently large, and then use the poitner
/// to that last element (an array to pointer decay) for accessing the element.
/// One specific example is:
///
/// @code{.c}
/// struct procfs_debuginfo {
///   uint64_t vaddr;
///   char path[1];
/// }
///
/// struct {
///  procfs_debuginfo info;
///  char buff[1024];
/// } map;
/// @endcode
///
/// Then this structure is often given to a system call, like 'ioctl' or
/// 'devctl' by giving the base address and the length of the structure, and the
/// operating system will then write the full path in this example, exceeding
/// the array boundaries (which won't overwrite memory, because sufficient
/// memory has been allocated on the stack as in this example).
///
/// The problem arises in that the C++ compiler may make assumptions that the
/// length of 'path' cannot be longer than one. It has been observed for
/// example, that:
///
/// @code{.c}
/// printf("%s %d\n", map.info.path, strlen(map.info.path));
/// @endcode
///
/// can return some counterintuitive results, such as printing the string as one
/// would expect, but the length is printed as zero (where the compiler does not
/// compute the length at run-time, it simply optimises the value to be always
/// zero!).
///
/// This structure can help avoid that problem, by allocating the memory for you
/// as needed, such that it would be rewritten as:
///
/// @code{.cpp}
/// using procfs_debuginfo_tx = osbuff<procfs_debuginfo, 1024>;
/// procfs_debuginfo_tx map{};
/// devctl(fd, DCMD_PROC_MAPDEBUG_BASE, &map(), sizeof(map), nullptr);
/// return std::string{map().path};
/// @endcode
///
/// @tparam T the operating system type that is to be wrapped.
///
/// @tparam N the extra space to be allocated at the end of the buffer.
template <typename T, std::size_t N>
class osbuff {
 public:
  osbuff() = default;
  osbuff(const osbuff&) = default;
  auto operator=(const osbuff&) -> osbuff& = default;
  osbuff(osbuff&&) = delete;
  auto operator=(osbuff&&) -> osbuff& = delete;
  ~osbuff() = default;

  /// @brief Get a reference to the underlying storage as the OS type.
  ///
  /// @return a reference to the operating system object.
  auto operator()() -> T& {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return *reinterpret_cast<T*>(&buffer_);
  }

  /// @brief Get a constant reference to the underlying storage as the OS type.
  ///
  /// @return a constant reference to the operating system object.
  auto operator()() const -> const T& {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return *reinterpret_cast<T*>(&buffer_);
  }

 private:
  alignas(T) std::array<std::uint8_t, sizeof(T) + N> buffer_{};
};

/// @brief Get the name of the process for the given pid.
///
/// The name of the process may be a full path, a partial path, or only the name
/// of the executable.
///
/// @param pid the PID of the process to get the name for.
///
/// @return the name of the process.
auto get_proc_name(pid_t pid) -> stdext::expected<std::string, int>;

/// @brief Get the page size of the system.
///
/// @return the page size, in bytes, or an error.
auto get_syspage_size() -> stdext::expected<unsigned int, int>;

}  // namespace ubench::os

#endif