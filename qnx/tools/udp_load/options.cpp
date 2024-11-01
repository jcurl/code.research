#include "options.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <charconv>
#include <iostream>
#include <sstream>
#include <string_view>

namespace {

/// @brief Parses the string as an unsigned integer.
///
/// @tparam T The type to convert from the string to.
///
/// @param arg The argument given to the program which is to be parsed to an
/// integer.
///
/// @param value [out] The result of the parsing
///
/// @return true if the conversion was successful, false otherwise.
template <typename T>
auto parse_unsigned(std::string_view arg, T& value) -> bool {
  auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);
  if (ec != std::errc{}) return false;
  if (ptr != arg.data() + arg.size()) return false;
  return true;
}

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

    std::uint16_t port = 0;
    std::string_view portstr(arg.data() + port_sep + 1,
                             arg.size() - port_sep - 1);
    if (!parse_unsigned(portstr, port)) return false;
    addr.sin_port = htons(port);

    // Get a NUL-terminated string.
    std::string addrstr{arg.data(), port_sep};
    if (inet_pton(AF_INET, addrstr.data(), &(addr.sin_addr)) != 1) return false;
  } else {
    addr.sin_port = 0;
    if (inet_pton(AF_INET, arg.data(), &(addr.sin_addr)) != 1) return false;
  }

  addr.sin_family = AF_INET;
  return true;
}

void print_help(std::string_view prog_name) {
  std::cout << prog_name << " [-n<slots>] [-m<width>] [-p<packets>] [-s<size>]"
            << std::endl;
  std::cout << "  [-d<duration>] [-T<threads>] [-I] -S<sourceip> -D<destip>"
            << std::endl;
  std::cout << std::endl;
  std::cout << "Writes UDP packets bound from <sourceip> IPv4 address "
               "to <destip>."
            << std::endl;
  std::cout << std::endl;
  std::cout << " -B<mode> - sending mode: sendto; sendmmsg. Default is 'sendto'"
            << std::endl;
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

auto inet_ntos(const struct sockaddr_in& addr) -> const std::string {
  // C/C++ interop with the OS.
  //
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  char paddr[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &(addr.sin_addr), (char*)paddr, sizeof(paddr)) ==
      nullptr)
    return std::string{};

  std::stringstream result;
  result << std::string{(char*)paddr} << ":" << ntohs(addr.sin_port);
  return std::string{result.str()};
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
options::options(int& argc, char* const argv[]) noexcept {
  int c = 0;
  bool help = false;

  std::string_view prog_name{};
  if (argc >= 1) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    prog_name = std::string_view(argv[0]);
  } else {
    prog_name = std::string_view("udp_load");
  }

  while ((c = getopt(argc, argv, "n:m:p:s:d:B:T:S:D:I?")) != -1) {
    switch (c) {
      case 'n': {
        std::uint16_t n = 0;
        if (!parse_unsigned(std::string_view{optarg}, n)) {
          std::cerr << "Error: Invalid value for the number of slots - "
                    << optarg << std::endl;
          return;
        }
        if (n < 1) {
          std::cerr << "Error: Invalid value for the number of slots, must be "
                       "one or more, got "
                    << n << std::endl;
          return;
        }
        slots_ = n;
        break;
      }
      case 'm': {
        std::uint16_t m = 0;
        if (!parse_unsigned(std::string_view{optarg}, m)) {
          std::cerr << "Error: Invalid value for width - " << optarg
                    << std::endl;
          return;
        }
        if (m < 1) {
          std::cerr << "Error: Invalid value for the slot width, must be one "
                       "or more, got "
                    << m << std::endl;
          return;
        }
        width_ = m;
        break;
      }
      case 'p': {
        std::uint32_t p = 0;
        if (!parse_unsigned(std::string_view{optarg}, p)) {
          std::cerr << "Error: Invalid value for packets per window - "
                    << optarg << std::endl;
          return;
        }
        if (p < 1) {
          std::cerr << "Error: Invalid value for packets per window, must be "
                       "one or more, got "
                    << p << std::endl;
          return;
        }
        packets_ = p;
        break;
      }
      case 's': {
        std::uint16_t s = 0;
        if (!parse_unsigned(std::string_view{optarg}, s)) {
          std::cerr << "Error: Invalid value for packet size - " << optarg
                    << std::endl;
          return;
        }
        size_ = s;
        break;
      }
      case 'd': {
        std::uint32_t t = 0;
        if (!parse_unsigned(std::string_view{optarg}, t)) {
          std::cerr << "Error: Invalid value for duration - " << optarg
                    << std::endl;
          return;
        }
        if (t < 1000) {
          std::cerr << "Error: Invalid value for duration, should be at least "
                       "1000ms, got "
                    << t << std::endl;
          return;
        }
        duration_ = t;
        break;
      }
      case 'B': {
        auto arg = std::string_view{optarg};
        if (arg == "sendto") {
          mode_ = send_mode::mode_sendto;
        } else if (arg == "sendmmsg") {
          mode_ = send_mode::mode_sendmmsg;
        } else {
          std::cerr << "Error: Invalid test mode " << arg << std::endl;
          return;
        }
        break;
      }
      case 'T': {
        std::uint16_t t = 0;
        if (!parse_unsigned(std::string_view{optarg}, t)) {
          std::cerr << "Error: Invalid value for number of threads - " << optarg
                    << std::endl;
          return;
        }
        if (t < 1 || t > 255) {
          std::cerr << "Error: Invalid value for number of threads, should be "
                       "in the range 1..255, got "
                    << t << std::endl;
          return;
        }
        threads_ = t;
        break;
      }
      case 'S': {
        if (!parse_sockaddr(std::string_view{optarg}, source_)) {
          std::cerr << "Error: Invalid source address - " << optarg
                    << std::endl;
          return;
        }
        break;
      }
      case 'D': {
        if (!parse_sockaddr(std::string_view{optarg}, dest_)) {
          std::cerr << "Error: Invalid destination address - " << optarg
                    << std::endl;
          return;
        }
        break;
      }
      case 'I':
        idle_ = true;
        break;
      case '?':
        help = true;
        break;
      case ':':
        std::cerr << "Error: Option -" << optopt << " requires an operand"
                  << std::endl;
        help = true;
        break;
      default:
        std::cerr << "Error: Unknown option -" << optopt << std::endl;
        help = true;
        break;
    }
  }

  if (help) {
    if (is_valid_) std::cerr << std::endl;
    print_help(prog_name);
    return;
  }

  if (source_.sin_addr.s_addr == 0) {
    std::cerr << "Error: No source address provided with option -S"
              << std::endl;
    return;
  }
  if (dest_.sin_addr.s_addr == 0) {
    std::cerr << "Error: No destination address provided with option -D"
              << std::endl;
    return;
  }

  is_valid_ = true;
}
