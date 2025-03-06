#ifndef BENCHMARK_STRINTERN_READBUFF_H
#define BENCHMARK_STRINTERN_READBUFF_H

#include <array>
#include <filesystem>

#include "stdext/expected.h"
#include "ubench/file.h"

class readbuff {
 public:
  readbuff(std::filesystem::path path);
  readbuff(const readbuff&) = delete;
  auto operator=(const readbuff&) -> readbuff& = delete;
  readbuff(readbuff&&) = default;
  auto operator=(readbuff&&) -> readbuff& = default;
  ~readbuff() = default;

  /// @brief Get the next token.
  ///
  /// @return The token, unless there is an error.
  [[nodiscard]] auto get_token() -> stdext::expected<std::string_view, int>;

 private:
  std::size_t pos_{};
  std::size_t len_{};
  std::array<char, 65536> buff_{};
  ubench::file::fdesc file_{};
  bool eof_{};
  int errno_{};

  auto read_buff(std::size_t pos) -> std::size_t;
};

#endif
