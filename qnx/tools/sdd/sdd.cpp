#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "ubench/net.h"
#include "ubench/string.h"
#include "options.h"
#include "payload.h"

auto main(int argc, char* argv[]) -> int {
  auto options = make_options(argc, argv);
  if (!options) return options.error();

  std::cout << "Source: " << ubench::net::inet_ntos(options->source_addr())
            << std::endl;
  std::cout << "Source Intf: " << options->source_iface() << std::endl;
  std::cout << "Destination: " << ubench::net::inet_ntos(options->dest_addr())
            << std::endl;
  std::cout << "Interval: " << options->interval().count() << "ms" << std::endl;

  payload p{options->dest_addr()};
  while (true) {
    if (options->source_addr().sin_addr.s_addr) {
      p.query(options->source_addr());
    } else {
      if (options->source_iface().empty()) {
        p.query(options->source_addr().sin_port);
      } else {
        p.query(options->source_iface(), options->source_addr().sin_port);
      }
    }

    if (!p) {
      // No interfaces registered, so wait 5 seconds and do the query again.
      std::this_thread::sleep_for(std::chrono::seconds(5));
    } else {
      auto current = std::chrono::steady_clock::now();
      while (std::chrono::steady_clock::now() - current <
             std::chrono::seconds(30)) {
        p.send();
        std::this_thread::sleep_for(options->interval());
      }
    }
  }

  return 0;
}
