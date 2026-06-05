#ifndef SDD_OPTIONS_H
#define SDD_OPTIONS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <chrono>

#include "stdext/expected.h"

class options {
 public:
  options(const options&) = delete;
  auto operator=(const options&) -> options& = delete;
  options(options&&) = default;
  auto operator=(options&&) -> options& = default;
  ~options() = default;

  /// @brief Get the user provided source port and address.
  ///
  /// This is the '-S ipv4addr:port' option. The port is optional and zero will
  /// be returned if not provided, which code should choose a suitable default.
  ///
  /// @return The user provided source port and address. If any parameter is not
  /// provided, the field contains 0. Code should determine a suitable default
  /// for itself.
  [[nodiscard]] auto source_addr() const noexcept -> const struct sockaddr_in& {
    return source_;
  }

  /// @brief Get the user provided destination port and address.
  ///
  /// This is the '-D ipv4addr:port' option. The port is optional and zero will
  /// be returned if not provided, which code should choose a suitable default.
  ///
  /// @return The user provided destination port and address. If any parameter
  /// is not provided, the field contains 0. Code shold determine a suitable
  /// default for itself.
  [[nodiscard]] auto dest_addr() const noexcept -> const struct sockaddr_in& {
    return dest_;
  }

  /// @brief Get the user provided interval for updates.
  ///
  /// @return The interval, in units of milliseconds.
  [[nodiscard]] auto interval() const noexcept -> std::chrono::milliseconds {
    return interval_;
  }

 private:
  options() = default;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  friend auto make_options(int argc, char* const argv[]) noexcept
      -> stdext::expected<options, int>;

  struct sockaddr_in source_ {};
  struct sockaddr_in dest_ {};
  std::chrono::milliseconds interval_{1000};
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
[[nodiscard]] auto make_options(int argc, char* const argv[]) noexcept
    -> stdext::expected<options, int>;

#endif
