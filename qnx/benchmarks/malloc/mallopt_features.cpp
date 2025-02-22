#include "config.h"

#include <malloc.h>

#include <charconv>
#include <iostream>
#include <map>
#include <optional>
#include <string_view>
#include <tuple>

#include "stdext/expected.h"
#include "mallopt.h"

namespace {

static const std::map<std::string_view, int> mallopts {
#if HAVE_M_ARENA_MAX
  {"M_ARENA_MAX", M_ARENA_MAX},
#endif
#if HAVE_M_ARENA_TEST
      {"M_ARENA_TEST", M_ARENA_TEST},
#endif
#if HAVE_M_MMAP_MAX
      {"M_MMAP_MAX", M_MMAP_MAX},
#endif
#if HAVE_M_MMAP_THRESHOLD
      {"M_MMAP_THRESHOLD", M_MMAP_THRESHOLD},
#endif
#if HAVE_M_MXFAST
      {"M_MXFAST", M_MXFAST},
#endif
#if HAVE_M_PERTURB
      {"M_PERTURB", M_PERTURB},
#endif
#if HAVE_M_TOP_PAD
      {"M_TOP_PAD", M_TOP_PAD},
#endif
#if HAVE_M_TRIM_THRESHOLD
      {"M_TRIM_THRESHOLD", M_TRIM_THRESHOLD},
#endif

#if HAVE_MALLOC_ARENA_CACHE_FREE_NOW
      {"MALLOC_ARENA_CACHE_FREE_NOW", MALLOC_ARENA_CACHE_FREE_NOW},
#endif
#if HAVE_MALLOC_ARENA_CACHE_MAXBLK
      {"MALLOC_ARENA_CACHE_MAXBLK", MALLOC_ARENA_CACHE_MAXBLK},
#endif
#if HAVE_MALLOC_ARENA_CACHE_MAXSZ
      {"MALLOC_ARENA_CACHE_MAXSZ", MALLOC_ARENA_CACHE_MAXSZ},
#endif
#if HAVE_MALLOC_ARENA_SIZE
      {"MALLOC_ARENA_SIZE", MALLOC_ARENA_SIZE},
#endif
#if HAVE_MALLOC_FREE_CHECK
      {"MALLOC_FREE_CHECK", MALLOC_FREE_CHECK},
#endif
#if HAVE_MALLOC_MAX_ALIGNMENT
      {"MALLOC_MAX_ALIGNMENT", MALLOC_MAX_ALIGNMENT},
#endif
#if HAVE_MALLOC_MEMORY_HOLD
      {"MALLOC_MEMORY_HOLD", MALLOC_MEMORY_HOLD},
#endif
#if HAVE_MALLOC_VERIFY
      {"MALLOC_VERIFY", MALLOC_VERIFY},
#endif
#if HAVE_MALLOC_VERIFY_ON
      {"MALLOC_VERIFY_ON", MALLOC_VERIFY_ON},
#endif

#if HAVE_M_GRANULARITY
      {"M_GRANULARITY", M_GRANULARITY},
#endif
};

auto trim(std::string_view sv) -> std::string_view {
  sv.remove_prefix(std::min(sv.find_first_not_of(' '), sv.size()));
  sv.remove_suffix(sv.size() - (sv.find_last_not_of(' ') + 1));
  return sv;
}

auto mallopt_find(std::string_view mallopt_item) -> std::optional<int> {
  auto search = mallopts.find(mallopt_item);
  if (search == mallopts.end()) return {};

  return search->second;
}

auto parse_mallopt(std::string_view mallopt_item)
    -> stdext::expected<std::tuple<int, int, std::string>, std::string> {
  auto kvp_separator = mallopt_item.find('=');
  if (kvp_separator == std::string_view::npos)
    return stdext::unexpected{"Missing value"};
  if (kvp_separator == mallopt_item.size() - 1)
    return stdext::unexpected{"Value not provided"};

  auto key = trim(mallopt_item.substr(0, kvp_separator));
  if (key.size() == 0) return stdext::unexpected{"Key name empty"};

  auto value_str = trim(mallopt_item.substr(kvp_separator + 1));
  if (value_str.size() == 0) return stdext::unexpected{"Value empty"};

  int value = 0;
  auto [ptr, ec] = std::from_chars(
      value_str.data(), value_str.data() + value_str.size(), value);
  if (ec != std::errc{}) return stdext::unexpected{"Value not a number"};

  auto mallopt_param = mallopt_find(key);
  if (!mallopt_param) {
    return stdext::unexpected{"Unknown key"};
  }

  return std::make_tuple(mallopts.at(key), value, std::string{key});
}

}  // namespace

namespace impl {

auto parse_mallopt_arg(std::string_view mallopt_arg)
    -> std::vector<std::tuple<int, int, std::string>> {
  std::vector<std::tuple<int, int, std::string>> mallopts{};
  std::string_view::size_type last{std::string_view::npos};
  do {
    std::string_view::size_type start = last + 1;
    last = mallopt_arg.find(',', start);

    std::string_view mallopt_item;
    if (last != std::string_view::npos) {
      mallopt_item = mallopt_arg.substr(start, last);
    } else {
      mallopt_item = mallopt_arg.substr(start);
    }

    auto opt = parse_mallopt(mallopt_item);
    if (!opt) {
      std::cout << "Invalid option -m" << mallopt_item << " (" << opt.error()
                << ")" << std::endl;
      return {};
    }

    mallopts.emplace_back(*opt);
  } while (last != std::string_view::npos);
  return mallopts;
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PRINT_OPTION(feature_name)                 \
  if (HAVE_##feature_name) {                       \
    std::cout << " - " #feature_name << std::endl; \
  }

auto print_mallopt_help() -> void {
  std::cout << "Options supported by '-m' are:" << std::endl;
  PRINT_OPTION(M_ARENA_MAX)
  PRINT_OPTION(M_ARENA_TEST)
  PRINT_OPTION(M_MMAP_MAX)
  PRINT_OPTION(M_MMAP_THRESHOLD)
  PRINT_OPTION(M_MXFAST)
  PRINT_OPTION(M_PERTURB)
  PRINT_OPTION(M_TOP_PAD)
  PRINT_OPTION(M_TRIM_THRESHOLD)
  PRINT_OPTION(MALLOC_ARENA_CACHE_FREE_NOW)
  PRINT_OPTION(MALLOC_ARENA_CACHE_MAXBLK)
  PRINT_OPTION(MALLOC_ARENA_CACHE_MAXSZ)
  PRINT_OPTION(MALLOC_ARENA_SIZE)
  PRINT_OPTION(MALLOC_FREE_CHECK)
  PRINT_OPTION(MALLOC_MAX_ALIGNMENT)
  PRINT_OPTION(MALLOC_MEMORY_HOLD)
  PRINT_OPTION(MALLOC_VERIFY)
  PRINT_OPTION(MALLOC_VERIFY_ON)
  PRINT_OPTION(M_GRANULARITY)
}

}  // namespace impl