#include "str_intern.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "ubench/measure/busy_measurement.h"
#include "ubench/measure/print.h"
#include "ubench/str_intern.h"
#include "allocator.h"
#include "options.h"
#include "readbuff.h"

auto print_stats(const options& options, const mem_metrics& metrics,
    const ubench::measure::busy_measurement& end, std::size_t words,
    std::size_t interned) -> void {
  ubench::measure::table table{};

  table.add_column("Metric");
  table.add_column(options.strintern_s(), ubench::measure::alignment::right);

  table.add_line({"Words", std::to_string(words)});
  table.add_line({"Interned Words", std::to_string(interned)});
  table.add_line(
      {"Currently Allocated (bytes)", std::to_string(metrics.current_alloc)});
  table.add_line(
      {"Max Allocated Mem (bytes)", std::to_string(metrics.max_alloc)});
  table.add_line(
      {"Total Allocated Mem (bytes)", std::to_string(metrics.total_alloc)});
  table.add_line(
      {"Total Freed Mem (bytes)", std::to_string(metrics.total_free)});
  table.add_line({"Allocation Count", std::to_string(metrics.allocs)});
  table.add_line({"Free Count", std::to_string(metrics.frees)});
  table.add_line({"Process Time (ms)", std::to_string(end.cpu_time.count())});
  table.add_line({"System Time (ms)", std::to_string(end.busy_time.count())});
  table.add_line({"Elapsed Time (ms)", std::to_string(end.run_time.count())});
  std::cout << table << std::endl;
}

auto main(int argc, char* argv[]) -> int {
  auto options = make_options(argc, argv);
  if (!options) return options.error();

  readbuff buff{options->path()};
  ubench::measure::busy_stop_watch stopwatch{};
  reset_alloc();

  std::unique_ptr<str_intern> intern{};
  switch (options->strintern()) {
    case strintern_impl::none:
      intern = std::make_unique<intern_none>();
      break;
    case strintern_impl::flist:
      intern = std::make_unique<intern_forward_list>();
      break;
    case strintern_impl::set:
      intern = std::make_unique<intern_ordered_set>();
      break;
    case strintern_impl::unordered_set:
      intern = std::make_unique<intern_unordered_set>();
      break;
    case strintern_impl::fixed_set_128k:
      intern = std::make_unique<intern_fixed_set>(131072);
      break;
    case strintern_impl::fixed_set_256k:
      intern = std::make_unique<intern_fixed_set>(262144);
      break;
    case strintern_impl::fixed_set_512k:
      intern = std::make_unique<intern_fixed_set>(524288);
      break;
    case strintern_impl::fixed_set_1m:
      intern = std::make_unique<intern_fixed_set>(1048576);
      break;
    case strintern_impl::var_set:
      // 8 million buckets. Use the default load factor of 1.0.
      intern = std::make_unique<intern_var_set>(4096, 1 << 23);
      break;
    case strintern_impl::var_set_pmr:
      // 8 million buckets. Use the default load factor of 1.0.
      intern = std::make_unique<intern_var_set_pmr>(4096, 1 << 23);
      break;
    case strintern_impl::ubench:
      break;
    default:
      std::cerr << "Unknown intern implementation to test with." << std::endl;
      return 2;
  }

  std::size_t w = 0;
  if (intern) {
    while (true) {
      auto token = buff.get_token();
      if (!token) break;
      w++;
      intern->intern(*token);
    }
    auto metrics = get_stats();
    auto end = stopwatch.measure();
    print_stats(*options, metrics, end, w, intern->size());
  } else {
    // the library doesn't implement the abstract class, which was intended for
    // testing only.
    ubench::string::str_intern uintern{};
    while (true) {
      auto token = buff.get_token();
      if (!token) break;
      w++;
      uintern.intern(*token);
    }
    auto metrics = get_stats();
    auto end = stopwatch.measure();
    print_stats(*options, metrics, end, w, uintern.size());
  }

  return 0;
}
