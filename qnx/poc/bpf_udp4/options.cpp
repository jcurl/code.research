#include "options.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <string_view>

#include "stdext/expected.h"
#include "ubench/string.h"

namespace {

/// @brief Parses the string in the format of an IPv4 number, a colon and a port
///
/// @param arg The argument given to the program which is to be parsed to an
/// IPv4 number and port.
///
/// @param addr [out] The result of parsing. If no port is present in the
/// string, it is set to zero.
///
/// @return true if the conversion was successful, false otherwise.
auto parse_sockaddr(std::string_view arg, sockaddr_in& addr) -> bool {
  auto port_sep = arg.find_last_of(':');
  if (port_sep != std::string_view::npos) {
    if (arg.size() < port_sep - 1) return false;

    std::string_view portstr(
        arg.data() + port_sep + 1, arg.size() - port_sep - 1);
    auto port = ubench::string::parse_int<std::uint16_t>(portstr);
    if (!port) return false;
    addr.sin_port = htons(*port);

    // Get a NUL-terminated string. The value `port_sep` is guaranteed to be
    // to be within the bounds of the string.
    //
    // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage)
    std::string addrstr{arg.data(), port_sep};
    if (inet_pton(AF_INET, addrstr.data(), &(addr.sin_addr)) != 1) return false;
  } else {
    addr.sin_port = 0;
    // arg may not be NUL terminated. Ensure it is.
    std::string addrstr{arg.data(), arg.size()};
    if (inet_pton(AF_INET, addrstr.data(), &(addr.sin_addr)) != 1) return false;
  }

  addr.sin_family = AF_INET;
  return true;
}

void print_help(std::string_view prog_name) {
  std::cout << prog_name << " -S<sourceip> -D<destip> [-B<mode>] [-s<size>]"
            << std::endl;
  std::cout << std::endl;
  std::cout << "Writes a UDP packet bound from <sourceip> IPv4 address "
               "to <destip>."
            << std::endl;
  std::cout << std::endl;

  std::cout << " -S<sourceip> - Source IP address (must be an existing "
               "interface)."
            << std::endl;
  std::cout << " -D<destip> - Destination IP address (can be unicast "
               "or multicast)."
            << std::endl;
  std::cout << " -B<mode> - How to send the packet {bpf, sendto}." << std::endl;
  std::cout << " -s<size> - Size of each UDP packet (default 1472)."
            << std::endl;
  std::cout << std::endl;
  std::cout << " -? - Display this help." << std::endl;
}

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
auto make_options(int& argc, char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  int c = 0;
  bool help = false;
  int err = 0;

  std::string_view prog_name{};
  if (argc >= 1) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    prog_name = std::string_view(argv[0]);
  } else {
    prog_name = std::string_view("udp_load");
  }

  options o{};
  while ((c = getopt(argc, argv, "s:S:D:B:?")) != -1) {
    switch (c) {
      case 'B': {
        auto arg = std::string_view{optarg};
        if (arg == "sendto") {
          o.mode_ = send_mode::mode_sendto;
        } else if (arg == "bpf") {
          o.mode_ = send_mode::mode_bpf;
        } else {
          std::cerr << "Error: Invalid test mode " << arg << std::endl;
          return stdext::unexpected{1};
        }
        break;
      }
      case 's': {
        auto s = ubench::string::parse_int<std::uint16_t>(optarg);
        if (!s) {
          std::cerr << "Error: Invalid value for packet size - " << optarg
                    << std::endl;
          return stdext::unexpected{1};
        }
        if (*s > 65527) {
          std::cerr << "Error: Size of packet cannot be more than 65527 bytes."
                    << std::endl;
          return stdext::unexpected{1};
        }
        o.size_ = *s;
        break;
      }
      case 'S': {
        if (!parse_sockaddr(std::string_view{optarg}, o.source_)) {
          std::cerr << "Error: Invalid source address - " << optarg
                    << std::endl;
          return stdext::unexpected{1};
        }
        break;
      }
      case 'D': {
        if (!parse_sockaddr(std::string_view{optarg}, o.dest_)) {
          std::cerr << "Error: Invalid destination address - " << optarg
                    << std::endl;
          return stdext::unexpected{1};
        }
        break;
      }
      case '?':
        if (optopt) err = 1;
        help = true;
        break;
      case ':':
        std::cerr << "Error: Option -" << optopt << " requires an operand"
                  << std::endl;
        err = 1;
        break;
      default:
        std::cerr << "Error: Unknown option -" << optopt << std::endl;
        err = 1;
        break;
    }
  }

  if (err || help) {
    print_help(prog_name);
    return stdext::unexpected{err};
  }

  if (o.source_.sin_addr.s_addr == 0) {
    std::cerr << "Error: No source address provided with option -S"
              << std::endl;
    return stdext::unexpected{1};
  }
  if (o.dest_.sin_addr.s_addr == 0) {
    std::cerr << "Error: No destination address provided with option -D"
              << std::endl;
    return stdext::unexpected{1};
  }

  return o;
}
