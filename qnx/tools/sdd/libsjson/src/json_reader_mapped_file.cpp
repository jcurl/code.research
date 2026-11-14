#include "json_reader_mapped_file.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cerrno>

#include "ubench/file.h"

namespace ubench::sjson::detail {

mapped_file::~mapped_file() {
  if (data_ != nullptr) ::munmap(data_, size_);
}

auto mapped_file::open(const std::filesystem::path& path)
    -> stdext::expected<mapped_file, int> {
  // The descriptor is closed when fd goes out of scope. The mapping stays valid
  // after that.
  ubench::file::fdesc fd{path.string(), O_RDONLY | O_CLOEXEC};
  if (!fd) return stdext::unexpected{errno};

  struct stat info {};
  if (fstat(fd, &info) != 0) return stdext::unexpected{errno};

  auto size = static_cast<std::size_t>(info.st_size);
  if (size == 0) {
    // mmap() rejects a zero length, and an empty file has nothing to map.
    return mapped_file{nullptr, 0};
  }

  void* data = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data == MAP_FAILED) return stdext::unexpected{errno};

  return mapped_file{data, size};
}

}  // namespace ubench::sjson::detail
