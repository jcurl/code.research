#include <sys/procmgr.h>

#include <iostream>
#include <thread>

#include "osqnx/resmgr/device.h"
#include "ubench/string.h"
#include "cpuid-driver.h"

auto driver_thread(os::qnx::resmgr::device<cpuid_driver>& driver,
    stdext::expected<int, int>& result) noexcept -> void {
  result = driver.run();
  if (!result) {
    if (result.error() > 0) {
      std::cout << "Driver Exit Error: "
                << ubench::string::perror(result.error()) << std::endl;
    } else {
      std::cout << "Driver Exit Error: " << result.error() << std::endl;
    }
  } else {
    std::cout << "Exiting: " << *result << std::endl;
  }
}

int main(void) {
  stdext::expected<int, int> result{};
  os::qnx::resmgr::device<cpuid_driver> driver{};

  // Start the thread, and wait for the driver to run. If it's not running,
  // there was an error.
  std::thread thread(
      [&driver, &result]() -> void { driver_thread(driver, result); });
  driver.wait();
  if (!driver) {
    thread.join();
    return 1;
  }

  // Detach, that the process that called us thinks we're done. We continue to
  // run our process in the background on our thread, and just wait for it to
  // exit.
  // procmgr_daemon(0, 0);
  procmgr_daemon(0, PROCMGR_DAEMON_NOCHDIR | PROCMGR_DAEMON_NOCLOSE |
                        PROCMGR_DAEMON_NODEVNULL | PROCMGR_DAEMON_KEEPUMASK);
  thread.join();
  return 0;
}
