#include "cpuid/cpuidreader_dev.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <memory>
#include <string>

#include "stdext/expected.h"

namespace rjcp::cpuid {

namespace detail {

cpuid_dev_file::cpuid_dev_file(unsigned int core, token) : core_{core} {
  std::string path = "/dev/cpu/" + std::to_string(core) + "/cpuid";
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  fd_ = open(path.c_str(), O_RDONLY);
  if (fd_ < 0) {
    error_ = errno;
  }

  cpuid_ = cpuid_dev{fd_};
}

auto cpuid_dev_file::close() -> bool {
  int result = 0;
  if (fd_ >= 0) {
    result = ::close(fd_);
    fd_ = -1;
  }

  return result == 0;
}

}  // namespace detail

auto cpuidreader_dev::has_cpuid() -> bool {
  if (cpuid_file_ == nullptr || !(*cpuid_file_)) {
    // Create for default core 0, if we have no context open.
    cpuid_file_ = std::make_unique<detail::cpuid_dev_file>(
        0, detail::cpuid_dev_file::token{});
    default_ = true;
  }
  return *cpuid_file_;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto cpuidreader_dev::cpuid(cpuidreg eax, cpuidreg ecx)
    -> std::optional<cpuid_res> {
  if (cpuid_file_ == nullptr || !(*cpuid_file_)) {
    // Create for default core 0, if we have no context open.
    cpuid_file_ = std::make_unique<detail::cpuid_dev_file>(
        0, detail::cpuid_dev_file::token{});
    default_ = true;
  }
  return cpuid_file_->cpuid().cpuid(eax, ecx);
}

auto cpuidreader_dev::enable_core(unsigned int core)
    -> stdext::expected<std::unique_ptr<cpuid_ctx>, int> {
  if (cpuid_file_ != nullptr && *cpuid_file_) {
    if (default_) {
      cpuid_file_->close();
    } else {
      return stdext::unexpected{EINVAL};
    }
  }
  default_ = false;
  cpuid_file_ = std::make_shared<detail::cpuid_dev_file>(
      core, detail::cpuid_dev_file::token{});
  if (!(*cpuid_file_)) return stdext::unexpected{cpuid_file_->error()};

  return std::make_unique<detail::cpuid_dev_ctx>(
      cpuid_file_, detail::cpuid_dev_ctx::token{});
}

}  // namespace rjcp::cpuid
