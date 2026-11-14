#ifndef SJSON_JSON_READER_MAPPED_FILE_H
#define SJSON_JSON_READER_MAPPED_FILE_H

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string_view>
#include <utility>

#include "stdext/expected.h"

namespace ubench::sjson::detail {

/// @brief A file mapped read-only into memory.
///
/// The mapping is released when the last owner is destroyed. Owners share it
/// through a std::shared_ptr, so copies never remap the file.
class mapped_file {
 public:
  mapped_file(const mapped_file&) = delete;
  auto operator=(const mapped_file&) -> mapped_file& = delete;
  mapped_file(mapped_file&& other) noexcept
      : data_{std::exchange(other.data_, nullptr)},
        size_{std::exchange(other.size_, 0)} {}
  auto operator=(mapped_file&& other) noexcept -> mapped_file& {
    // The temporary takes this object's old mapping and unmaps it on exit.
    mapped_file released{std::move(other)};
    std::swap(data_, released.data_);
    std::swap(size_, released.size_);
    return *this;
  }
  ~mapped_file();

  /// @brief Map the file at the path read-only.
  /// @param path The file to map.
  /// @return The mapping. An empty file has an empty view and no mapping. On
  /// failure, the unix error code (errno value) from the failing call.
  static auto open(const std::filesystem::path& path)
      -> stdext::expected<mapped_file, int>;

  /// @brief The contents of the file.
  /// @return A view of the mapped memory, valid while this object exists.
  auto view() const noexcept -> std::string_view {
    return {static_cast<const char*>(data_), size_};
  }

 private:
  mapped_file(void* data, std::size_t size) noexcept
      : data_{data}, size_{size} {}

  void* data_{};
  std::size_t size_{};
};

}  // namespace ubench::sjson::detail

#endif
