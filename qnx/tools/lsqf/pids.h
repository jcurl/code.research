#ifndef LSQF_PIDS_H
#define LSQF_PIDS_H

#include <map>
#include <optional>
#include <string>
#include <vector>

class pids {
 public:
  pids() = default;
  pids(const pids&) = default;
  auto operator=(const pids&) -> pids& = default;
  pids(pids&&) = default;
  auto operator=(pids&&) -> pids& = default;

  /// @brief Gets the name of the process for the given PID.
  ///
  /// If the PID name is not cached, then it is queried by the Operating
  /// System.
  ///
  /// @param pid The PID of the process to obtain.
  ///
  /// @return The name of the process.
  auto get_name(int pid, bool short_path = false) -> std::optional<std::string>;

  /// @brief Drop the PID from the cache.
  ///
  /// Remove the PID from the cache. This is useful on indication that the PID
  /// has terminated.
  ///
  /// @param pid The PID to drop from the cache.
  ///
  /// @return true if the PID was present and is removed.
  auto drop(int pid) -> bool;

  /// @brief Query the OS for the list of all PIDs.
  ///
  /// @return A vector of all PIDs known in the system at the time.
  auto get_pids() -> std::vector<int>;

 private:
  std::map<int, std::string> pid_names_{};
};

#endif
