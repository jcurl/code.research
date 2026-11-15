#include <csignal>
#include <iostream>

#include "cli_presenter.h"
#include "sdd/local_computer_service.h"

auto main() -> int {
  // Block before start() so the service thread inherits the mask and signals
  // are only consumed by sigwait().
  sigset_t signals;
  sigfillset(&signals);
  pthread_sigmask(SIG_BLOCK, &signals, nullptr);

  sdd::core::local_computer_service service{sdd::core::local_computer_service_config{}};
  sdd::cli::cli_presenter presenter{service};
  if (!service.start()) {
    std::cerr << "sddmoncli: failed to start the local computer service\n";
    return 1;
  }

  return presenter.run();
}
