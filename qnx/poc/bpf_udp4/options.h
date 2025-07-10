#ifndef POC_BPF_OPTIONS_H
#define POC_BPF_OPTIONS_H

#include <netinet/in.h>

#include <cstdint>

#include "stdext/expected.h"
#include "udp_socket.h"

/// @brief User options.
class options {
 public:
  options(const options&) = delete;
  auto operator=(const options&) -> options& = delete;
  options(options&&) = default;
  auto operator=(options&&) -> options& = default;
  ~options() = default;

  /// @brief The test mode that should be used.
  ///
  /// This is the '-B mode' option that defines the underlying API for sending
  /// traffic.
  ///
  /// @return Returns the test mode that should be used.
  [[nodiscard]] auto mode() const noexcept -> send_mode { return mode_; }

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

  /// @brief Get the size of the packet that should be used.
  ///
  /// This is the '-s size' option, which is the size of the payload used in the
  /// IPv4 UDP packet. The user is expected to know the MTU for which data is
  /// being sent to take into account if the stack will accept the packet or not
  /// (or cause fragmentation).
  ///
  /// @return The size of the payload that should be used in a packet. In case
  /// the value is invalid, zero is returned. The default value is 1472 bytes,
  /// which is the maximum size of a UDP packet over Ethernet.
  [[nodiscard]] auto size() const noexcept -> std::uint16_t { return size_; }

 private:
  options() = default;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  friend auto make_options(int argc, const char* const argv[]) noexcept
      -> stdext::expected<options, int>;

  send_mode mode_{send_mode::mode_sendto};
  struct sockaddr_in source_ {};
  struct sockaddr_in dest_ {};
  std::uint16_t size_{1472};
};

/// @brief Get options.
///
/// The command line options are parsed and the fields of this class are updated
/// accordingly. In case the user provides an error, or a value out of range for
/// an option, then this class will automatically tell the user of the error on
/// the console.
///
/// In cases where no option is provided, either a default value will be given
/// (as documented in the method), or zero will be returned indicating that no
/// option was provided.
///
/// If the user provides '-?' then help is printed.
///
/// @param argc [in, out] Reference to the number of arguments
///
/// @param argv [in, out] Pointer to the argument vector array
///
/// @return The options object, or an error code. An error code of zero
/// indicates no options, but the user requested help.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
[[nodiscard]] auto make_options(int argc, const char* const argv[]) noexcept
    -> stdext::expected<options, int>;

#endif
