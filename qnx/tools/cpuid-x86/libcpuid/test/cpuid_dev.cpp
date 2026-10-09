#include "cpuid_dev.h"

#include <filesystem>
#include <string>

auto has_cpuid_dev_driver() -> bool {
  auto fd = get_cpuid_dev_fdpath(0);
  return static_cast<bool>(fd);
}

auto get_cpuid_dev_fdpath(unsigned int cpunum) -> ubench::file::fdesc {
  std::string path =
      std::string{"/dev/cpu/"} + std::to_string(cpunum) + std::string{"/cpuid"};
  std::filesystem::path dev{path};

  if (!std::filesystem::exists(dev)) return ubench::file::fdesc{};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  ubench::file::fdesc fd = open(dev.c_str(), O_RDONLY | O_CLOEXEC);
  return fd;
}
