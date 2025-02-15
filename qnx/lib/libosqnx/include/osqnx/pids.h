#ifndef LIB_OSQNX_PIDS_H
#define LIB_OSQNX_PIDS_H

#include <map>
#include <string>
#include <vector>

#include "stdext/expected.h"

namespace os::qnx {

/// @brief Maintain a list of avialable PIDs in the system.
class pids {
 public:
  pids() = default;
  pids(const pids&) = default;
  auto operator=(const pids&) -> pids& = default;
  pids(pids&&) = default;
  auto operator=(pids&&) -> pids& = default;
  virtual ~pids() = default;

  /// @brief Gets the name of the process for the given PID.
  ///
  /// If the PID name is not cached, then it is queried by the Operating
  /// System.
  ///
  /// @param pid The PID of the process to obtain.
  ///
  /// @return The name of the process.
  auto get_name(unsigned int pid, bool short_path = false)
      -> stdext::expected<std::string, int>;

  /// @brief Drop the PID from the cache.
  ///
  /// Remove the PID from the cache. This is useful on indication that the PID
  /// has terminated.
  ///
  /// @param pid The PID to drop from the cache.
  ///
  /// @return true if the PID was present and is removed.
  auto drop(unsigned int pid) -> bool;

  /// @brief Query the OS for the list of all PIDs.
  ///
  /// This method queries the folder /proc for all PIDs. It doesn't cache the
  /// available PIDs. The returned object is a temporary object.
  ///
  /// @return A vector of all PIDs known in the system at the time.
  auto query_pids() -> std::vector<unsigned int>;

 private:
  std::map<unsigned int, std::string> pid_names_{};
};

}  // namespace os::qnx

#endif
