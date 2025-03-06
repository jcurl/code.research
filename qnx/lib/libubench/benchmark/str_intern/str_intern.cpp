#include "str_intern.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "ubench/measure/busy_measurement.h"
#include "ubench/measure/print.h"
#include "allocator.h"
#include "options.h"
#include "readbuff.h"

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
    default:
      std::cerr << "Unknown intern implementation to test with." << std::endl;
      return 2;
  }

  unsigned int w = 0;
  while (true) {
    auto token = buff.get_token();
    if (!token) break;
    w++;
    intern->intern(*token);
  }

  auto metrics = get_stats();
  auto end = stopwatch.measure();

  ubench::measure::table table{};

  table.add_column("Metric");
  table.add_column(options->strintern_s(), ubench::measure::alignment::right);

  table.add_line({"Words", std::to_string(w)});
  table.add_line({"Interned Words", std::to_string(intern->size())});
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

  return 0;
}
