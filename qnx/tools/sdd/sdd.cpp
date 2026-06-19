#include <iostream>

#include "ubench/net.h"
#include "options.h"
#include "payload.h"

auto main(int argc, char* argv[]) -> int {
  auto options = make_options(argc, argv);
  if (!options) return options.error();

  std::cout << "Source: " << ubench::net::inet_ntos(options->source_addr())
            << std::endl;
  std::cout << "Destination: " << ubench::net::inet_ntos(options->dest_addr())
            << std::endl;
  std::cout << "Interval: " << options->interval().count() << "ms" << std::endl;

  auto p = payload{options->source_addr()};
  std::cout << p.generate();

  return 0;
}