#include <chrono>
#include <iostream>
#include <thread>

#include "ubench/net.h"
#include "ubench/string.h"
#include "options.h"
#include "payload.h"
#include "udp.h"

auto main(int argc, char* argv[]) -> int {
  auto options = make_options(argc, argv);
  if (!options) return options.error();

  std::cout << "Source: " << ubench::net::inet_ntos(options->source_addr())
            << std::endl;
  std::cout << "Destination: " << ubench::net::inet_ntos(options->dest_addr())
            << std::endl;
  std::cout << "Interval: " << options->interval().count() << "ms" << std::endl;

  payload p{};
  auto ores = p.open(options->source_addr(), options->dest_addr());
  if (!ores) return ores.error();

  while (true) {
    auto sres = p.send();
    if (!sres) {
      std::cout << "Error sending: " << ubench::string::perror(sres.error());
      return sres.error();
    }
    std::this_thread::sleep_for(options->interval());
  }

  return 0;
}