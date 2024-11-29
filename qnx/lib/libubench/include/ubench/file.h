#ifndef UBENCH_FILE_H
#define UBENCH_FILE_H

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <string>

namespace ubench::file {

/// @brief Represents an OS file (file descriptor).
///
/// Wrap an operating system filedescriptor in a class, so that when it goes out
/// of scope, it is automatically closed.
///
/// The copy constructor and copy assignment are deleted, so that there cannot
/// be more than one instance of the same file descriptor. Use the implicit
/// constructor accepting an integer (the returned file descriptor).
class fdesc {
 public:
  /// @brief Instantiate an os_file from an integer file descriptor
  ///
  /// Operations that return a file descriptor can be given to this class. When
  /// this class goes out of scope, the file descriptor is closed.
  ///
  /// @param fd the file descriptor to manage.
  fdesc(int fd) noexcept : fd_{fd} {}

  /// @brief Open using the open() system call, read only.
  ///
  /// @param path The filesystem path to open.
  explicit fdesc(std::string path) noexcept {
    fd_ = open(path.c_str(), O_RDONLY);
  }

  /// @brief Open using the open() system call.
  ///
  /// @param path The filesystem path to open.
  ///
  /// @param flags The flags to use when opening the file.
  explicit fdesc(std::string path, int flags) noexcept {
    fd_ = open(path.c_str(), flags);
  }

  fdesc(const fdesc&) = delete;
  auto operator=(const fdesc&) -> fdesc& = delete;

  /// @brief Move constructor.
  ///
  /// @param other The other object being moved to here.
  fdesc(fdesc&& other) {
    fd_ = other.fd_;
    other.fd_ = -1;
  }

  /// @brief Move assignment operator.
  ///
  /// If this object already contains a file descriptor, it is closed. The other
  /// object is moved here, and the other objects file descriptor is set to
  /// invalid.
  ///
  /// @param other The other object being moved here.
  ///
  /// @return a reference to this object.
  auto operator=(fdesc&& other) -> fdesc& {
    if (fd_ != -1) close(fd_);

    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }

  /// @brief Get if the file descriptor is valid (not -1).
  operator bool() const { return fd_ != -1; }

  /// @brief Get the file descriptor.
  ///
  /// @return a temporary copy of the file descriptor.
  operator int() const { return fd_; }

  /// @brief Set this object to invalid, without closing the file descriptor.
  ///
  /// @return the file descriptor before it is reset.
  auto reset() -> int {
    fd_ = -1;
    return fd_;
  }

  /// @brief Closes the file descriptor.
  ~fdesc() {
    if (fd_ != -1) close(fd_);
    fd_ = -1;
  }

 private:
  int fd_{-1};
};

}  // namespace ubench::file

#endif