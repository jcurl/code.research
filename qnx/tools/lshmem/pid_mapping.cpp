#include "pid_mapping.h"

#include <sys/mman.h>

#include <cerrno>
#include <cstdint>
#include <fstream>
#include <locale>
#include <memory>
#include <tuple>

#include "stdext/expected.h"
#include "ubench/str_intern.h"
#include "ubench/string.h"

namespace {

enum col : std::size_t {
  paddr,   //< mi_paddr
  pgsize,  //< mi_pgsize
  flags,   //< ri_flags
  prot,    //< ri_prot
  dev,     //< dev
  ino,     //< ino
  object   //< object_name
};

const std::vector<std::string> columns{"mi_paddr", "mi_pgsize", "ri_flags",
    "ri_prot", "dev", "ino", "object_name"};

auto find_columns(
    const std::string& headline, const std::vector<std::string>& map_fields)
    -> std::tuple<std::size_t, std::vector<std::size_t>> {
  auto fields = ubench::string::split_args(headline);
  if (fields.size() < map_fields.size()) return {};

  std::size_t found{};
  std::vector<std::size_t> index(map_fields.size());
  for (std::size_t f = 0; f < fields.size(); f++) {
    for (std::size_t c = 0; c < map_fields.size(); c++) {
      if (fields[f] == map_fields[c]) {
        // We assume that no entry in `map_fields` is duplicate.
        index[c] = f;
        found++;

        // Skip this loop.
        c = map_fields.size();
      }
    }
  }
  if (found != map_fields.size()) index.clear();
  return std::make_tuple(fields.size(), index);
}

template <typename T>
auto get_hex(std::string_view field, bool& valid) -> T {
  if (!valid) return {};
  if (field.size() < 3) {
    valid = false;
    return {};
  }
  if (field.substr(0, 2) != "0x") {
    valid = false;
    return {};
  }
  T value;
  auto [ptr, ec] = ubench::string::from_chars_hex<T>(
      field.data() + 2, field.data() + field.size(), value);
  if (ec != std::errc{} || ptr != field.data() + field.size()) {
    valid = false;
  }
  return value;
}

auto inline is_mapped(std::uintptr_t ptr) -> bool {
  return ptr != ~std::uintptr_t{};
}

auto inline is_shared(int flags) -> bool {
  return (flags & (MAP_SHARED | MAP_PRIVATE | MAP_ELF | MAP_STACK)) ==
         MAP_SHARED;
}

auto inline is_writable(int prot) -> bool {
  return ((prot << 8) & PROT_WRITE) == PROT_WRITE;
}

auto inline is_same(const map_line& l1, const map_line& l2) -> bool {
  return l1.ri_flags == l2.ri_flags && l1.ri_prot == l2.ri_prot &&
         l1.dev == l2.dev && l1.ino == l2.ino && l1.object == l2.object;
}

auto inline merge_mapline(map_line& line, map_line& merge) -> bool {
  if (line.phys_addr + line.phys_len == merge.phys_addr &&
      is_same(line, merge)) {
    line.phys_len += merge.phys_len;
    return true;
  }

  if (merge.phys_addr + merge.phys_len == line.phys_addr &&
      is_same(merge, line)) {
    line.phys_addr -= merge.phys_len;
    line.phys_len += merge.phys_len;
    return true;
  }

  return false;
}

}  // namespace

auto pid_mapping::insert(map_line&& line) -> void {
  bool first = true;
  for (auto it = map_.begin(); it != map_.end(); it = std::next(it)) {
    map_line& entry = *it;

    // The entry already exists, we have a duplicate.
    if (line.phys_addr == entry.phys_addr) return;

    // Insert line before this entry for it to be sorted by phys_addr.
    if (line.phys_addr < entry.phys_addr) {
      // Check if entry + line is the same.
      if (merge_mapline(entry, line)) {
        if (!first) {
          // Check if prev + entry is the same.
          map_line& prev = *std::prev(it);
          if (merge_mapline(entry, prev)) {
            map_.erase(std::prev(it));
          }
        }
      } else {
        if (!first) {
          // Check of prev + line is the same.
          map_line& prev = *std::prev(it);
          if (merge_mapline(prev, line)) return;
        }
        // Otherwise new entry that can't be merged.
        line.object = strings_->intern(line.object);
        map_.insert(it, std::move(line));
      }
      return;
    }
    first = false;
  }

  // It must be put at the end (a copy).
  if (!first) {
    map_line& prev = *std::prev(map_.end());
    if (merge_mapline(prev, line)) return;
  }
  line.object = strings_->intern(line.object);
  map_.push_back(std::move(line));
}

auto load_mapping(std::filesystem::path mapping, bool read,
    std::shared_ptr<ubench::string::str_intern> strings)
    -> stdext::expected<pid_mapping, int> {
  std::ifstream file{};
  file.open(mapping);
  if (!file) {
    if (file.fail()) return stdext::unexpected{errno};
    return stdext::unexpected{EINVAL};
  }

  // Ensure that we always interpret the file correctly irrespective of
  // whatever locale the user has configured.
  file.imbue(std::locale::classic());

  // Get the title line. Then map the column to the field in the title line.
  std::string headline;
  if (!std::getline(file, headline)) {
    if (file.fail()) return stdext::unexpected{errno};
    return stdext::unexpected{EINVAL};
  }
  auto [count, col_index] = find_columns(headline, columns);
  if (col_index.size() == 0) return stdext::unexpected{EINVAL};

  // Parse each memory map line.
  pid_mapping map{std::move(strings)};
  while (true) {
    std::string line;
    if (!std::getline(file, line)) {
      if (file.eof()) return map;
      if (file.fail()) return stdext::unexpected{errno};
      return stdext::unexpected{EINVAL};
    }

    auto fields = ubench::string::split_args(line, count);
    if (fields.size() < count) {
      // There are less fields than the title.
      continue;
    }

    bool valid = true;
    map_line map_line{// initialise inline
        .phys_addr =
            get_hex<std::uintptr_t>(fields[col_index[col::paddr]], valid),
        .phys_len = get_hex<std::size_t>(fields[col_index[col::pgsize]], valid),
        .ri_flags = get_hex<int>(fields[col_index[col::flags]], valid),
        .ri_prot = get_hex<int>(fields[col_index[col::prot]], valid),
        .dev = get_hex<dev_t>(fields[col_index[col::dev]], valid),
        .ino = get_hex<ino_t>(fields[col_index[col::ino]], valid),
        .object = fields[col_index[col::object]]};

    // We only care about physically mapped, shared memory.
    if (valid && is_mapped(map_line.phys_addr) &&
        is_shared(map_line.ri_flags) &&
        (read || is_writable(map_line.ri_prot))) {
      map.insert(std::move(map_line));
    }
  }
}
