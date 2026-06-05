#include "options.h"

#include <iostream>

#include "ubench/net.h"
#include "ubench/options.h"
#include "ubench/string.h"

namespace {

const std::uint16_t default_port = 52599;
const std::uint32_t default_dest = 0xEFFF2A63;

void print_help(std::string_view prog_name) {
  std::cout << prog_name << " [-S <sourceip>] [-D <destip>] [-n <time>]"
            << std::endl;
  std::cout << std::endl;
  std::cout << "Writes UDP packets bound from <sourceip> IPv4 address "
               "to <destip>."
            << std::endl;
  std::cout << "at regular intervals defined by <time>." << std::endl;
  std::cout << std::endl;

  std::cout << " -S <sourceip> - Source IP address (must be an existing "
               "interface)."
            << std::endl;
  std::cout << " -D <destip> - Destination IP address (can be unicast "
               "or multicast)."
            << std::endl;

  std::cout << " -n <time> - Interval in seconds for sending packets."
            << std::endl;
  std::cout << " -? - Display this help." << std::endl;
}

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
auto make_options(int argc, char* const argv[]) noexcept
    -> stdext::expected<options, int> {
  options o{};
  ubench::options opts{argc, argv, "n:S:D:?"};
  for (const auto& opt : opts) {
    if (opt) {
      switch (opt->get_option()) {
        case 'n': {
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          auto arg = *opt->argument();
          auto n = ubench::string::parse_int<unsigned int>(arg);
          if (!n) {
            std::cerr << "Error: Invalid value for time - " << arg << std::endl;
            return stdext::unexpected{1};
          }
          if (*n < 1 || *n > 86400) {
            std::cerr
                << "Error: Invalid value for the number of slots, must be "
                   "1..86400 seconds, got "
                << *n << std::endl;
            return stdext::unexpected{1};
          }

          o.interval_ = std::chrono::milliseconds{*n * 1000};
          break;
        }
        case 'S': {
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          auto arg = *opt->argument();
          if (!ubench::net::parse_sockaddr(std::string_view{arg}, o.source_)) {
            std::cerr << "Error: Invalid source address - " << arg << std::endl;
            return stdext::unexpected{1};
          }
          break;
        }
        case 'D': {
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          auto arg = *opt->argument();
          if (!ubench::net::parse_sockaddr(std::string_view{arg}, o.dest_)) {
            std::cerr << "Error: Invalid destination address - " << arg
                      << std::endl;
            return stdext::unexpected{1};
          }
          break;
        }
        case '?':
          print_help(opts.prog_name());
          return stdext::unexpected{0};
        default:
          ubench::options::print_error(opt->get_option());
          return stdext::unexpected{1};
      }
    } else {
      ubench::options::print_error(opt.error());
      return stdext::unexpected{1};
    }
  }

  if (o.source_.sin_family != AF_INET) {
    // Option "-S" wasn't provided
    o.source_.sin_family = AF_INET;
    o.source_.sin_addr.s_addr = 0;
    o.source_.sin_port = htons(default_port);
  }

  if (o.dest_.sin_family != AF_INET) {
    // Option "-D" wasn't provided
    o.dest_.sin_family = AF_INET;
    o.dest_.sin_addr.s_addr = htonl(default_dest);
    o.dest_.sin_port = htons(default_port);
  }

  if (o.source_.sin_port == 0) {
    o.source_.sin_port = htons(default_port);
  }

  if (o.dest_.sin_addr.s_addr == 0) {
    o.dest_.sin_addr.s_addr = htonl(default_dest);
  }

  if (o.dest_.sin_port == 0) {
    o.dest_.sin_port = htons(default_port);
  }

  return o;
}
