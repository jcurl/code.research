#ifndef BENCHMARK_MALLOPT_H
#define BENCHMARK_MALLOPT_H

#include <string_view>

class mallopt_options {
 public:
  explicit mallopt_options(int argc, char **argv);
  mallopt_options(const mallopt_options &) = delete;
  mallopt_options(mallopt_options &&) = delete;
  auto operator=(const mallopt_options &) -> mallopt_options & = delete;
  auto operator=(mallopt_options &&) -> mallopt_options & = delete;

  auto result() -> int;
  auto do_run() -> bool;

 private:
  auto parse_mallopt_arg(std::string_view mallopt_arg) -> bool;
  auto print_help(std::string_view prog_name) -> void;
  auto print_mallopt_help() -> void;

  int result_{0};
  bool do_run_{true};
};

#endif