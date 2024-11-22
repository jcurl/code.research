#include <malloc.h>

#include <charconv>
#include <cstring>
#include <iostream>
#include <map>
#include <optional>
#include <string_view>

#include "mallopt.h"

#include "config.h"

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
    -> std::optional<std::string_view> {
  auto kvp_separator = mallopt_item.find('=');
  if (kvp_separator == std::string_view::npos) return "Missing value";
  if (kvp_separator == mallopt_item.size() - 1) return "Value not provided";

  auto key = trim(mallopt_item.substr(0, kvp_separator));
  if (key.size() == 0) return "Key name empty";

  auto value_str = trim(mallopt_item.substr(kvp_separator + 1));
  if (value_str.size() == 0) return "Value empty";

  int value = 0;
  auto [ptr, ec] = std::from_chars(
      value_str.data(), value_str.data() + value_str.size(), value);
  if (ec != std::errc{}) return "Value not a number";

  auto mallopt_param = mallopt_find(key);
  if (!mallopt_param) {
    return "Unknown key";
  }

  std::cout << "Setting mallopt(" << key << ", " << value << ");" << std::endl;
#ifdef __QNX__
  // Under QNX: 0 on success, or -1 if an error occurs (errno is set).
  int opt = mallopts.at(key);
  int result = mallopt(mallopts.at(key), value);
  if (opt == MALLOC_ARENA_SIZE) {
    // For `MALLOC_ARENA_SIZE`, if value is 0, `mallopt()` returns the current
    // arena size; if value is any other value, `mallopt()` returns the previous
    // size.
    if (value) return {};
  }
  if (result) {
    if (errno) return strerror(errno);
    std::cout << key << " returns " << value << ". Continuing." << std::endl;
  }
#else
  // Under Linux: On success, mallopt() returns 1.  On error, it returns 0.
  int result = mallopt(mallopts.at(key), value);
  if (!result) return strerror(errno);
#endif
  return {};
}

}  // namespace

auto mallopt_options::parse_mallopt_arg(std::string_view mallopt_arg) -> bool {
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

    auto error = parse_mallopt(mallopt_item);
    if (error) {
      std::cout << "Invalid option -m" << mallopt_item << " (" << error.value()
                << ")" << std::endl;
      return false;
    }
  } while (last != std::string_view::npos);
  return true;
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PRINT_OPTION(feature_name)                 \
  if (HAVE_##feature_name) {                       \
    std::cout << " - " #feature_name << std::endl; \
  }

auto mallopt_options::print_mallopt_help() -> void {
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