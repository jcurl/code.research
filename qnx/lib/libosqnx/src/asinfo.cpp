#include "osqnx/asinfo.h"

#include <sys/syspage.h>

#include <algorithm>
#include <memory>

namespace {

std::unique_ptr<os::qnx::asinfo_map> asinfo_{};

// The functions to access the 'asinfo' must be in the global namespace, so we
// avoid conflicts with the macros.

auto inline get_asinfo_cbegin() -> const asinfo_entry* {
  return SYSPAGE_ENTRY(asinfo);
}

auto inline get_asinfo_cend() -> const asinfo_entry* {
  int assize = SYSPAGE_ENTRY_SIZE(asinfo);
  int elsize = SYSPAGE_ELEMENT_SIZE(asinfo);
  return SYSPAGE_ARRAY_IDX(asinfo, assize / elsize);
}

auto inline get_asinfo_cnext(const asinfo_entry* item) -> const asinfo_entry* {
  int elsize = SYSPAGE_ELEMENT_SIZE(asinfo);
  return SYSPAGE_ARRAY_ADJ_OFFSET(asinfo, item, elsize);
}

auto inline get_asinfo_size() -> std::size_t {
  int assize = SYSPAGE_ENTRY_SIZE(asinfo);
  int elsize = SYSPAGE_ELEMENT_SIZE(asinfo);
  return assize / elsize;
}

}  // namespace

namespace os::qnx {

auto get_asinfo() -> const asinfo_map& {
  if (asinfo_) return *asinfo_;

  asinfo_map map{};

  const auto* last = get_asinfo_cend();
  const auto* first = get_asinfo_cbegin();
  while (first != last) {
    if ((first->attr & AS_ATTR_KIDS) == 0) {
      asinfo_entry entry{};
      entry.start = first->start;
      entry.end = first->end;
      entry.name = std::string_view{SYSPAGE_ENTRY(strings)->data + first->name};

      // Not the most efficient way to insert, but the number of entries are
      // expected to be less than 100, and this routine really only needs to be
      // called once by application code (because the syspage doesn't change
      // after boot).
      map.insert(
          std::upper_bound(map.begin(), map.end(), entry,
              [](const asinfo_entry& ins, const asinfo_entry& map_entry) {
                return ins.start < map_entry.start;
              }),
          entry);
    }
    first = get_asinfo_cnext(first);
  };
  asinfo_ = std::make_unique<asinfo_map>(map);
  return *asinfo_;
}

auto get_tymem_name(uintptr_t start) -> std::optional<std::string_view> {
  auto asinfo = get_asinfo();
  if (asinfo.size() == 0) return std::nullopt;

  std::size_t lower = 0;
  std::size_t upper = asinfo.size() - 1;
  while (true) {
    std::size_t mid = (lower + upper) / 2;
    if (asinfo[mid].in_range(start)) return asinfo[mid].name;

    // Don't use `.end`, as this may be smaller than `.start`.
    if (start >= asinfo[lower].start &&
        start <= asinfo[mid].start + asinfo[mid].size()) {
      upper = mid - 1;
    } else if (start > asinfo[mid].start &&
               start <= asinfo[upper].start + asinfo[upper].size()) {
      lower = mid + 1;
    } else {
      return std::nullopt;
    }
  }
}

auto get_sysram() -> std::size_t {
  auto asinfo = get_asinfo();
  if (asinfo.size() == 0) return 0;

  std::size_t memory{};
  for (const auto& entry : asinfo) {
    if (entry.name == "sysram") {
      memory += entry.size();
    }
  }
  return memory;
}

}  // namespace os::qnx