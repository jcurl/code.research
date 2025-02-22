#include "config.h"

#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include "core_benchmark.h"
#include "corerw_benchmark.h"
#include "options.h"

auto main(int argc, char* argv[]) -> int {
  auto options = make_options(argc, argv);
  if (!options) return options.error();

  std::unique_ptr<benchmark> bm{};
  switch (options->benchmark()) {
    case core_mode::mode_readwrite: {
      bm = std::make_unique<corerw_benchmark>(
          options->iters(), options->samples());
      break;
    }
    case core_mode::mode_cas: {
      auto cm = core_benchmark::mode(options->benchmark_name());
      if (cm) {
        bm = std::make_unique<core_benchmark>(
            options->iters(), options->samples(), *cm);
      }
      break;
    }
  }

  if (!bm) {
    // Shouldn't get here, unless a bug in options that didn't check properly.
    std::cerr << "Error: benchmark type unknown." << std::endl;
    return 1;
  }

  if (!bm->init()) {
    std::cout << "Cannot initialise benchmark " << bm->name() << std::endl;
    std::cout << " - Check if your processor has the features required."
              << std::endl;
    return 1;
  }

  // Title with cores.
  std::cout << "Running " << bm->name() << " Core Benchmark" << std::endl;
  std::cout << " Samples: " << options->samples() << std::endl;
  std::cout << " Iterations: " << options->iters() << std::endl;
  std::cout << std::endl;

  std::cout << "      ";
  for (unsigned int pong_core = 0;
       pong_core < std::thread::hardware_concurrency(); pong_core++) {
    std::cout << std::left << std::setw(5) << pong_core << " ";
  }
  std::cout << std::endl;

  for (unsigned int ping_core = 0;
       ping_core < std::thread::hardware_concurrency(); ping_core++) {
    std::cout << std::left << std::setw(5) << ping_core << " ";
    for (unsigned int pong_core = 0;
         pong_core < std::thread::hardware_concurrency(); pong_core++) {
      if (pong_core == ping_core) {
        std::cout << "      " << std::flush;
      } else {
        std::uint64_t time = bm->run(ping_core, pong_core);
        std::cout << std::left << std::setw(5) << time << " " << std::flush;
      }
    }
    std::cout << std::endl;
  }
  return 0;
}
