#ifndef BENCHMARK_UDP_OPTIONS_H
#define BENCHMARK_UDP_OPTIONS_H

#include <netinet/in.h>

#include <chrono>
#include <cstdint>
#include <string>

#include "udp_talker.h"

/// @brief Decode the command line and present the options to the user.
class options {
 public:
  /// @brief Constructor
  ///
  /// The command line options are parsed and the fields of this class are
  /// updated accordingly. In case the user provides an error, or a value out of
  /// range for an option, then this class will automatically tell the user of
  /// the error on the console. The is_valid() property will indicate false.
  ///
  /// In cases where no option is provided, either a default value will be given
  /// (as documented in the method), or zero will be returned indicating that no
  /// option was provided. As there was no error, the property is_valid() will
  /// return true.
  ///
  /// If the user provides '-?' then help is printed.
  ///
  /// @param argc [in, out] Reference to the number of arguments
  ///
  /// @param argv [in, out] Pointer to the argument vector array
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  options(int& argc, char* const argv[]) noexcept;
  options(const options& other) = delete;
  auto operator=(const options& other) -> options& = delete;
  options(options&& other) = delete;
  auto operator=(options&& other) -> options& = delete;
  ~options() = default;

  /// @brief Returns if the configuration options provided by the user is valid
  /// or not.
  ///
  /// @return true if the configuration is valid. false if the user provided an
  /// error in the arguments, and the program should abort.
  [[nodiscard]] auto is_valid() const noexcept -> bool { return is_valid_; }

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

  /// @brief Get the number of slots the user provided.
  ///
  /// This is the '-n slots' option. The number of slots should be a value of 1
  /// or higher.
  ///
  /// @return The number of slots that the user provided. In case the value is
  /// invalid, zero is returned. The default value is 20 slots.
  [[nodiscard]] auto slots() const noexcept -> std::uint16_t { return slots_; }

  /// @brief Get the width, in milliseconds, of each slot.
  ///
  /// This is the '-m width' option. The duration is in milliseconds, and
  /// defines the duration of each slot, thus defining a sliding window size of
  /// slots() * width() milliseconds.
  ///
  /// @return The width, in milliseconds, of each slot for the sliding window
  /// for traffic shaping. In case the value is invalid, zero is returned. The
  /// default value is 10ms.
  [[nodiscard]] auto width() const noexcept -> std::uint16_t { return width_; }

  /// @brief Get the size of each packet that should be used.
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

  /// @brief The number of packets that should be sent in a complete window.
  ///
  /// This is the '-p count' option. It defines the number of packets that
  /// should be sent over a period of time given by slots() * width(). If more
  /// than one thread is required (see the threads() property), then the packet
  /// size is divided across the number of threads, so that the expected number
  /// of packets over the window remains constant.
  ///
  /// Normally, this option is calculated by the user based on the slots() and
  /// the width() parameters provided.
  ///
  /// @return The number of apckets that should be sent in a complete window,
  /// which is defined as the duration of slots() * width() milliseconds. In
  /// case the vlaue is invalid, zero is returned. A default of 1000 packets is
  /// defined.
  [[nodiscard]] auto packets() const noexcept -> std::uint32_t {
    return packets_;
  }

  /// @brief The duration of the test in milliseconds.
  ///
  /// This is the '-d duration' option. The duration of the test, in
  /// millisecons. The default value is 30,000 milliseconds. The actual time
  /// required for the test is rounded up to the next slot.
  ///
  /// @return The duration of the test in milliseconds. In case this value is
  /// invalid, zero milliseconds is returned.
  [[nodiscard]] auto duration() const noexcept -> std::chrono::milliseconds {
    return std::chrono::milliseconds(duration_);
  }

  /// @brief The number of threads to use for the test.
  ///
  /// This is the '-T threads' option. The number of threads that should be set
  /// up for running the test. Each thread runs independently in parallel, with
  /// its own socket (and therefore file-descriptor). While there is no check on
  /// the maximum number of threads, it should be chosen that there is at least
  /// one packet per window for each thread (if not, then a thread will not be
  /// created). It should not exceed too many threads and some systems may limit
  /// the maximum number of threads that can be created. A hard limit of 256
  /// threads is enforced.
  ///
  /// @return The number of threads that should be used. The value is zero if
  /// the user provided an invalid value. The default is one thread. No maximum
  /// is encoded, so that a single core may have multiple threads running.
  /// Threads are not bound to a CPU core.
  [[nodiscard]] auto threads() const noexcept -> std::uint16_t {
    return threads_;
  }

  /// @brief Enable the IDLE test
  ///
  /// This is the '-I' option (no argument). The idle test is running for a few
  /// seconds to measure the idle load of the system in the background. If the
  /// measurement of the idle load is deemed inaccurate for the system, and the
  /// operator using the tool has a better mechanism for measuring the CPU load
  /// of the sytem while the measurement is running, the operator can disable
  /// the idle mode test.
  ///
  /// @return true if the idle test should run, false otherwise.
  [[nodiscard]] auto enable_idle_test() const noexcept -> bool { return idle_; }

 private:
  bool is_valid_{false};
  send_mode mode_{send_mode::mode_sendto};
  bool idle_{false};
  struct sockaddr_in source_ {};
  struct sockaddr_in dest_ {};
  std::uint16_t slots_{20};
  std::uint16_t width_{5};
  std::uint16_t size_{1472};
  std::uint32_t packets_{1000};
  std::uint32_t duration_{30000};
  std::uint16_t threads_{1};
};

/// @brief Converts addr to a string.
///
/// This is functionality equivalent to inet_ntop() for C++ (can't use this
/// function name as some systems won't compile if the names overlap).
///
/// @param addr The IPv4 address and port to stringify.
///
/// @return The string converted, or on error, an empty string.
auto inet_ntos(const struct sockaddr_in& addr) -> const std::string;

#endif
