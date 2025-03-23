#include "readbuff.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <string_view>

#include "stdext/expected.h"
#include "ubench/file.h"

// Implementing a very fast implementation that doesn't require buffering or
// read-ahead. Data is put into an array, so we'll use the Posix API. Using the
// Posix API directly allows better control over memory allocations.

readbuff::readbuff(std::filesystem::path path) {
  auto filename = path.string();

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  file_ = open(filename.c_str(), O_RDONLY);
  if (!file_) {
    errno_ = errno;
    return;
  }
}

auto readbuff::read_buff(std::size_t pos) -> std::size_t {
  if (eof_) return 0;

  std::size_t tr = 0;
  do {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    ssize_t r = read(file_, &buff_[pos], buff_.size() - pos);
    if (r == 0) {
      eof_ = true;
      return tr;
    }
    if (r < 0 && errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN) {
      eof_ = true;
      errno_ = errno;
      return tr;
    }
    pos += r;
    tr += r;
  } while (pos < buff_.size());
  return tr;
}

auto readbuff::get_token() -> stdext::expected<std::string_view, int> {
  if (pos_ == len_) {
    pos_ = 0;
    len_ = read_buff(0);
    if (len_ == 0) return stdext::unexpected{errno_};
  }

  // Look for the first non-space character.
  std::size_t p{};
  bool reading = true;
  while (reading) {
    p = pos_;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    while (p < len_ && isspace(buff_[p])) {
      p++;
    }
    if (p < len_) {
      reading = false;
    } else {
      // Used the buffer. Read it in again.
      pos_ = 0;
      len_ = read_buff(0);
      if (len_ == 0) return stdext::unexpected{errno_};
    }
  };

  // Look for the end of the word
  std::size_t q = p + 1;
  while (true) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    while (q < len_ && !isspace(buff_[q])) {
      q++;
    }
    if (p == 0 || q < len_) {
      // Start reading next time from one past the space.
      pos_ = q == len_ ? len_ : q + 1;
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      return std::string_view(&buff_[p], q - p);
    }
    // Read to the end and no space. So we shift the data to the beginning, read
    // it in, and continue.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    std::memmove(&buff_[0], &buff_[p], len_ - p);
    std::size_t r = read_buff(len_ - p);
    if (r == 0) {
      pos_ = len_;
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      return std::string_view(&buff_[0], len_ - p);
    }
    q = len_ - p;
    len_ = q + r;
    pos_ = p;
    p = 0;
  }
}
