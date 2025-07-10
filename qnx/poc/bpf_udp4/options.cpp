#include "options.h"

#include <sys/socket.h>
#include <arpa/inet.h>

#include <iostream>
#include <string_view>

#include "stdext/expected.h"
#include "ubench/options.h"
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
auto make_options(int argc, const char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  bool help = false;
  int err = 0;

  options o{};
  ubench::options opts{argc, argv, "s:S:D:B:?"};
  for (const auto& opt : opts) {
    if (opt) {
      switch (opt->get_option()) {
        case 'B': {
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          auto arg = std::string_view{*opt->argument()};
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
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          auto arg = *opt->argument();
          auto s = ubench::string::parse_int<std::uint16_t>(arg);
          if (!s) {
            std::cerr << "Error: Invalid value for packet size - " << arg
                      << std::endl;
            return stdext::unexpected{1};
          }
          if (*s > 65527) {
            std::cerr
                << "Error: Size of packet cannot be more than 65527 bytes."
                << std::endl;
            return stdext::unexpected{1};
          }
          o.size_ = *s;
          break;
        }
        case 'S': {
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          auto arg = *opt->argument();
          if (!parse_sockaddr(arg, o.source_)) {
            std::cerr << "Error: Invalid source address - " << arg << std::endl;
            return stdext::unexpected{1};
          }
          break;
        }
        case 'D': {
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          auto arg = *opt->argument();
          if (!parse_sockaddr(arg, o.dest_)) {
            std::cerr << "Error: Invalid destination address - " << arg
                      << std::endl;
            return stdext::unexpected{1};
          }
          break;
        }
        case '?':
          help = true;
          break;
        default:
          err = 1;
          ubench::options::print_error(opt->get_option());
          break;
      }
    } else {
      err = 1;
      ubench::options::print_error(opt.error());
    }
  }

  if (err || help) {
    if (err) std::cerr << std::endl;
    print_help(opts.prog_name());
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
