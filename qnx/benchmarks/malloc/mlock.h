#ifndef BENCHMARK_MLOCK_H
#define BENCHMARK_MLOCK_H

#include "stdext/expected.h"

auto enable_mlockall() -> stdext::expected<void, int>;

#endif