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
  std::cout << prog_name << " [-n<slots>] [-m<width>] [-p<packets>] [-s<size>]"
            << std::endl;
  std::cout << "  [-d<duration>] [-T<threads>] [-I] [-B<mode]" << std::endl;
  std::cout << "  -S<sourceip> -D<destip>" << std::endl;
  std::cout << std::endl;
  std::cout << "Writes UDP packets bound from <sourceip> IPv4 address "
               "to <destip>."
            << std::endl;
  std::cout << std::endl;

  std::cout << " -B<mode> - sending mode: ";
  bool option = false;
  if (make_udp_talker(send_mode::mode_sendto)->is_supported()) {
    if (option) std::cout << ", ";
    option = true;
    std::cout << "sendto";
  }
  if (make_udp_talker(send_mode::mode_sendmmsg)->is_supported()) {
    if (option) std::cout << ", ";
    option = true;
    std::cout << "sendmmsg";
  }
  if (make_udp_talker(send_mode::mode_bpf)->is_supported()) {
    if (option) std::cout << ", ";
    option = true;
    std::cout << "bpf";
  }
  if (option) {
    std::cout << "." << std::endl;
  } else {
    std::cout << "(none)." << std::endl;
  }

  std::cout << " -n<slots> - Number of slots in a time window (default 20)."
            << std::endl;
  std::cout << " -m<width> - Width of each slot in milliseconds (default 5ms)."
            << std::endl;
  std::cout << " -p<packets> - Number of packets to send in a window (n*m "
               "duration, default 1000)."
            << std::endl;
  std::cout << " -s<size> - Size of each UDP packet (default 1472)."
            << std::endl;
  std::cout << " -d<duration> - Duration to run the test in milliseconds "
               "(default 30,000ms)."
            << std::endl;
  std::cout << " -T<threads> - Number of parallel threads (default 1)."
            << std::endl;
  std::cout << " -S<sourceip> - Source IP address (must be an existing "
               "interface)."
            << std::endl;
  std::cout << " -D<destip> - Destination IP address (can be unicast "
               "or multicast)."
            << std::endl;
  std::cout << " -I - Enable IDLE mode test prior" << std::endl;
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
  while ((c = getopt(argc, argv, "n:m:p:s:d:B:T:S:D:I?")) != -1) {
    switch (c) {
      case 'n': {
        auto n = ubench::string::parse_int<std::uint16_t>(optarg);
        if (!n) {
          std::cerr << "Error: Invalid value for the number of slots - "
                    << optarg << std::endl;
          return stdext::unexpected{1};
        }
        if (*n < 1) {
          std::cerr << "Error: Invalid value for the number of slots, must be "
                       "one or more, got "
                    << *n << std::endl;
          return stdext::unexpected{1};
        }
        o.slots_ = *n;
        break;
      }
      case 'm': {
        auto m = ubench::string::parse_int<std::uint16_t>(optarg);
        if (!m) {
          std::cerr << "Error: Invalid value for width - " << optarg
                    << std::endl;
          return stdext::unexpected{1};
        }
        if (*m < 1) {
          std::cerr << "Error: Invalid value for the slot width, must be one "
                       "or more, got "
                    << *m << std::endl;
          return stdext::unexpected{1};
        }
        o.width_ = *m;
        break;
      }
      case 'p': {
        auto p = ubench::string::parse_int<std::uint32_t>(optarg);
        if (!p) {
          std::cerr << "Error: Invalid value for packets per window - "
                    << optarg << std::endl;
          return stdext::unexpected{1};
        }
        if (*p < 1) {
          std::cerr << "Error: Invalid value for packets per window, must be "
                       "one or more, got "
                    << *p << std::endl;
          return stdext::unexpected{1};
        }
        o.packets_ = *p;
        break;
      }
      case 's': {
        auto s = ubench::string::parse_int<std::uint16_t>(optarg);
        if (!s) {
          std::cerr << "Error: Invalid value for packet size - " << optarg
                    << std::endl;
          return stdext::unexpected{1};
        }
        o.size_ = *s;
        break;
      }
      case 'd': {
        auto t = ubench::string::parse_int<std::uint32_t>(optarg);
        if (!t) {
          std::cerr << "Error: Invalid value for duration - " << optarg
                    << std::endl;
          return stdext::unexpected{1};
        }
        if (*t < 1000) {
          std::cerr << "Error: Invalid value for duration, should be at least "
                       "1000ms, got "
                    << *t << std::endl;
          return stdext::unexpected{1};
        }
        o.duration_ = *t;
        break;
      }
      case 'B': {
        auto arg = std::string_view{optarg};
        if (arg == "sendto") {
          o.mode_ = send_mode::mode_sendto;
        } else if (arg == "sendmmsg") {
          o.mode_ = send_mode::mode_sendmmsg;
        } else if (arg == "bpf") {
          o.mode_ = send_mode::mode_bpf;
        } else {
          std::cerr << "Error: Invalid test mode " << arg << std::endl;
          return stdext::unexpected{1};
        }
        auto talker = make_udp_talker(o.mode_);
        if (!talker->is_supported()) {
          std::cerr << "Error: Unsupported test mode " << arg << std::endl;
          return stdext::unexpected{1};
        }
        break;
      }
      case 'T': {
        auto t = ubench::string::parse_int<std::uint16_t>(optarg);
        if (!t) {
          std::cerr << "Error: Invalid value for number of threads - " << optarg
                    << std::endl;
          return stdext::unexpected{1};
        }
        if (*t < 1 || *t > 255) {
          std::cerr << "Error: Invalid value for number of threads, should be "
                       "in the range 1..255, got "
                    << *t << std::endl;
          return stdext::unexpected{1};
        }
        o.threads_ = *t;
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
      case 'I':
        o.idle_ = true;
        break;
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
