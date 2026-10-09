#include "options.h"

#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>

#include <cerrno>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ubench/options.h"
#include "ubench/string.h"

namespace {

auto lookup_buffer_size(int name) -> std::size_t {
  long sz = sysconf(name);
  return sz > 0 ? static_cast<std::size_t>(sz) : 1024;
}

auto user_id_from_name(std::string name) -> std::optional<unsigned int> {
  std::vector<char> buf(lookup_buffer_size(_SC_GETPW_R_SIZE_MAX));

  passwd pwd{};
  passwd* result = nullptr;
  int err = getpwnam_r(name.c_str(), &pwd, buf.data(), buf.size(), &result);
  if (err != 0 || result == nullptr) return std::nullopt;
  return static_cast<unsigned int>(result->pw_uid);
}

auto group_id_from_name(std::string name) -> std::optional<unsigned int> {
  std::vector<char> buf(lookup_buffer_size(_SC_GETGR_R_SIZE_MAX));

  group grp{};
  group* result = nullptr;
  int err = getgrnam_r(name.c_str(), &grp, buf.data(), buf.size(), &result);
  if (err != 0 || result == nullptr) return {};
  return static_cast<unsigned int>(result->gr_gid);
}

}  // namespace

[[nodiscard]] auto make_options(int argc, const char* argv[]) noexcept
    -> stdext::expected<options, int> {
  options o{};
  ubench::options opts{argc, argv, "U:G:P:?"};
  for (const auto& opt : opts) {
    if (opt) {
      switch (opt->get_option()) {
        case 'G': {
          auto g = ubench::string::parse_int<unsigned int>(*opt->argument());
          if (!g) g = group_id_from_name(std::string{*opt->argument()});
          if (g) {
            o.group_id_ = *g;
          }
          break;
        }
        case 'U': {
          auto u = ubench::string::parse_int<unsigned int>(*opt->argument());
          if (!u) u = user_id_from_name(std::string{*opt->argument()});
          if (u) {
            o.user_id_ = *u;
          }
          break;
        }
        case 'P': {
          auto p =
              ubench::string::parse_int_octal<unsigned int>(*opt->argument());
          if (p) {
            o.permissions_ = *p;
          }
          break;
        }
        case '?': {
          std::cout << "devc-cpuid -U <uid> -G <gid> -P <perms>" << std::endl;
          return stdext::unexpected{0};
        }
        default:
          ubench::options::print_error(opt->get_option());
          return stdext::unexpected{1};
      }
    }
  }
  return o;
}
