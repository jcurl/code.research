#include "config.h"

#include <iostream>
#include <memory>

#include "ubench/string.h"
#include "bpf_socket.h"
#include "bsd_socket.h"
#include "options.h"
#include "udp_socket.h"

auto main(int argc, char* argv[]) -> int {
  auto options = make_options(argc, argv);
  if (!options) return options.error();

  std::unique_ptr<udp_socket> s{};

  switch (options->mode()) {
    case send_mode::mode_sendto:
      s = std::make_unique<bsd_socket>(
          options->source_addr(), options->dest_addr());
      break;
#if HAVE_BIOCSETIF
    case send_mode::mode_bpf:
      s = std::make_unique<bpf_socket>(
          options->source_addr(), options->dest_addr());
      break;
#endif
    default:
      std::cerr << "Mode not supported" << std::endl;
      return 1;
  }

  if (!s || !*s) {
    std::cerr << "Error initialising" << std::endl;
    return 1;
  }

  std::vector<std::byte> data(options->size());
  std::uint8_t d{0};
  for (auto& b : data) {
    b = static_cast<std::byte>(d);
    d++;
  }

  if (auto r = s->send(data); !r) {
    std::cerr << "Error sending packet - " << ubench::string::perror(r.error())
              << std::endl;
  }

  return 0;
}
