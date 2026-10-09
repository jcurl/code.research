#ifndef CPUID_DEVC_CPUID_OPTIONS_H
#define CPUID_DEVC_CPUID_OPTIONS_H

#include "stdext/expected.h"

class options {
 public:
  options(const options&) = delete;
  auto operator=(const options&) -> options& = delete;
  options(options&&) = default;
  auto operator=(options&&) -> options& = default;
  ~options() = default;

  [[nodiscard]] auto user_id() const noexcept -> unsigned int {
    return user_id_;
  }

  [[nodiscard]] auto group_id() const noexcept -> unsigned int {
    return group_id_;
  }

  [[nodiscard]] auto permissions() const noexcept -> unsigned int {
    return permissions_;
  }

 private:
  options() = default;
  friend auto make_options(int argc, const char* argv[]) noexcept
      -> stdext::expected<options, int>;

  unsigned int user_id_{0};
  unsigned int group_id_{0};
  unsigned int permissions_{0644};
};

[[nodiscard]] auto make_options(int argc, const char* argv[]) noexcept
    -> stdext::expected<options, int>;

#endif
