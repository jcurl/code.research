#ifndef UBENCH_FILE_H
#define UBENCH_FILE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <string>

namespace ubench::file {

/// @brief Represents an OS file (file descriptor).
///
/// Wrap an operating system file descriptor in a class, so that when it goes
/// out of scope, it is automatically closed.
///
/// The copy constructor and copy assignment are deleted, so that there cannot
/// be more than one instance of the same file descriptor. Use the implicit
/// constructor accepting an integer (the returned file descriptor).
///
/// When passing this around, either move it, or pass a reference to fdesc for
/// another function. A copy shouldn't duplicate the descriptor (this would be
/// confusing when passing to functions), nor should there be two instances used
/// at the same time. Don't create a second instance explicitly with the
/// underlying file descriptor, as it would be undefined behaviour to close that
/// file handle (the two instances would be managed independently and could lead
/// to common race conditions, such as use after free, or manipulate to the
/// wrong file).
class fdesc {
 public:
  fdesc() = default;

  /// @brief Instantiate an fdesc from an integer file descriptor.
  ///
  /// Operations that return a file descriptor can be given to this class. When
  /// this class goes out of scope, the file descriptor is closed.
  ///
  /// @param fd the file descriptor to manage.
  fdesc(int fd) noexcept : fd_{fd} {}

  /// @brief Assign an fdesc from an integer file descriptor.
  ///
  /// Close the existing file descriptor if already open before assigning.
  ///
  /// @param fd the file descriptor to manage.
  ///
  /// @return this object that was assigned.
  auto operator=(int fd) -> fdesc& {
    close();
    fd_ = fd;
    return *this;
  }

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
  fdesc(fdesc&& other) noexcept : fd_{other.fd_} { other.fd_ = -1; }

  /// @brief Move assignment operator.
  ///
  /// If this object already contains a file descriptor, it is closed. The other
  /// object is moved here, and the other objects file descriptor is set to
  /// invalid.
  ///
  /// @param other The other object being moved here.
  ///
  /// @return a reference to this object.
  auto operator=(fdesc&& other) noexcept -> fdesc& {
    if (fd_ != other.fd_) {
      close();
      fd_ = other.fd_;
    }
    other.fd_ = -1;
    return *this;
  }

  /// @brief Get if the file descriptor is valid (not -1).
  operator bool() const { return fd_ != -1; }

  /// @brief Get the file descriptor.
  ///
  /// @return a temporary copy of the file descriptor.
  operator int() const { return fd_; }

  /// @brief Close the socket and mark this descriptor as invalid.
  ///
  /// @return true if the handle was closed, false if it was already closed. If
  /// there was an error, it is ignored and it is assumed that the handle is now
  /// closed.
  auto close() -> bool {
    bool is_opened{fd_ != -1};
    if (is_opened) ::close(fd_);
    fd_ = -1;
    return is_opened;
  }

  /// @brief Set this object to invalid, without closing the file descriptor.
  ///
  /// @return the file descriptor before it is reset.
  auto reset() -> int {
    fd_ = -1;
    return fd_;
  }

  /// @brief Closes the file descriptor.
  ~fdesc() { close(); }

 private:
  int fd_{-1};
};

}  // namespace ubench::file

/// @brief Close the file descriptor, just the same way as the system close().
///
/// @return true if the handle was closed, false if it was already closed. If
/// there was an error, it is ignored and it is assumed that the handle is now
/// closed.
auto inline close(ubench::file::fdesc& fd) -> bool { return fd.close(); }

#endif