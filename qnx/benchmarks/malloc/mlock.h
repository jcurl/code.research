#ifndef BENCHMARK_MLOCK_H
#define BENCHMARK_MLOCK_H

#include <optional>
#include <string_view>

auto enable_mlockall() -> std::optional<std::string_view>;

#endif