#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <benchmark/benchmark.h>

#include "ubench/net.h"

// This benchmark is to explore the fastest way to convert a data structure to a
// string.
//
// The stream operators to a string is twice as slow as the "C" mechanism to
// write to a buffer.

// $ ./lib/libubench/benchmark/net_bench
// 2024-11-09T22:17:42+01:00
// Running ./lib/libubench/benchmark/net_bench
// Run on (8 X 3600 MHz CPU s)
// CPU Caches:
//   L1 Data 32 KiB (x4)
//   L1 Instruction 32 KiB (x4)
//   L2 Unified 256 KiB (x4)
//   L3 Unified 8192 KiB (x1)
// Load Average: 3.24, 1.53, 0.88
// ***WARNING*** CPU scaling is enabled, the benchmark real time measurements
// may be noisy and will incur extra overhead.
// --------------------------------------------------------------------
// Benchmark                          Time             CPU   Iterations
// --------------------------------------------------------------------
// BM_EtherToStreamFormat           372 ns          371 ns      1901580
// BM_EtherToStringFormat           318 ns          318 ns      2198153
// BM_EtherToStreamCpp              275 ns          275 ns      2552276
// BM_EtherToStringMove             524 ns          524 ns      1265996
// BM_EtherToStringReturn           524 ns          524 ns      1266042
// BM_SockAddrToStreamFormat        332 ns          332 ns      2080757
// BM_SockAddrToStringFormat        274 ns          274 ns      2532566
// BM_SockAddrToStreamCpp           244 ns          244 ns      2801388
// BM_SockAddrToStringMove          376 ns          376 ns      1889326
// BM_SockAddrToStringReturn        376 ns          376 ns      1853849

// Results
// - Choose a trade-off.
//   - Put the formatting in the `<<` operator:
//     - The `<<` operator is faster, but the string methods, which use
//       `stringstream` are ~ 2x slower than the stream. Note, this doesn't take
//       into account the fact that a stringstream can't reserve space upfront,
//       so may be slower due to unexpected memory allocations. We don't have
//       that problem with strings.
//   - Put the formatting in the function:
//     - The `<<` is slower, we create the string (via `std::snprintf`), then
//       write the string to the stream.
//
// Choice is to use the `std::snprintf` for creating the string which is fast.
// Then the stream operator is just a wrapper about writing the string.

namespace {

auto ether_ntos_format(const ubench::net::ether_addr& addr)
    -> const std::string {
  std::string result{};
  result.resize(18);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  int len = std::snprintf(result.data(), 18, "%02x:%02x:%02x:%02x:%02x:%02x",
      addr.ether_addr_octet[0], addr.ether_addr_octet[1],
      addr.ether_addr_octet[2], addr.ether_addr_octet[3],
      addr.ether_addr_octet[4], addr.ether_addr_octet[5]);
  result.resize(len);
  return result;
}

auto ether_stream_format(std::ostream& os, const ubench::net::ether_addr& addr)
    -> std::ostream& {
  return os << ether_ntos_format(addr);
}

auto ether_stream_cpp(std::ostream& os, const ubench::net::ether_addr& addr)
    -> std::ostream& {
  auto flags = os.flags();
  os << std::hex << std::setfill('0') << std::setw(2)
     << static_cast<int>(addr.ether_addr_octet[0]) << ":" << std::setw(2)
     << static_cast<int>(addr.ether_addr_octet[1]) << ":" << std::setw(2)
     << static_cast<int>(addr.ether_addr_octet[2]) << ":" << std::setw(2)
     << static_cast<int>(addr.ether_addr_octet[3]) << ":" << std::setw(2)
     << static_cast<int>(addr.ether_addr_octet[4]) << ":" << std::setw(2)
     << static_cast<int>(addr.ether_addr_octet[5]);
  os.flags(flags);
  return os;
}

auto ether_ntos_move(const ubench::net::ether_addr& addr) -> const std::string {
  std::stringstream result;
  ether_stream_cpp(result, addr);
  return std::move(*result.rdbuf()).str();
}

auto ether_ntos_return(const ubench::net::ether_addr& addr)
    -> const std::string {
  std::stringstream result;
  ether_stream_cpp(result, addr);
  return result.str();
}

auto inet_ntos_format(const sockaddr_in& addr) -> const std::string {
  // C/C++ interop with the OS.
  //
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  char paddr[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &(addr.sin_addr), (char*)paddr, sizeof(paddr)) ==
      nullptr)
    return {};

  std::string result{};
  result.resize(22);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  int len = std::snprintf(
      result.data(), 22, "%s:%d", (char*)paddr, ntohs(addr.sin_port));
  result.resize(len);
  return result;
}

auto sockaddr_stream_format(std::ostream& os, const sockaddr_in& addr)
    -> std::ostream& {
  return os << inet_ntos_format(addr);
}

auto sockaddr_stream_cpp(std::ostream& os, const sockaddr_in& addr)
    -> std::ostream& {
  // C/C++ interop with the OS.
  //
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  char paddr[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &(addr.sin_addr), (char*)paddr, sizeof(paddr)) ==
      nullptr)
    return os;

  return os << std::string{(char*)paddr} << ":" << ntohs(addr.sin_port);
}

auto inet_ntos_move(const sockaddr_in& addr) -> const std::string {
  std::stringstream result;
  sockaddr_stream_cpp(result, addr);
  return std::move(*result.rdbuf()).str();
}

auto inet_ntos_return(const sockaddr_in& addr) -> const std::string {
  std::stringstream result;
  sockaddr_stream_cpp(result, addr);
  return result.str();
}

const ubench::net::ether_addr addr{1, 0, 0x5e, 1, 2, 3};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
// We don't care about the missing initialisation of the padding.
const sockaddr_in ipaddr{.sin_family = AF_INET,
    .sin_port = 65535,
    .sin_addr = {.s_addr = 0x7f000001}};
#pragma GCC diagnostic pop

class NullBuffer : public std::streambuf {
 public:
  auto overflow(int c) -> int override { return c; }
};

// NOLINTBEGIN

void BM_EtherToStreamFormat(benchmark::State& state) {
  NullBuffer null_buffer;
  std::ostream null_stream(&null_buffer);

  for (auto _ : state) {
    ether_stream_format(null_stream, addr);
  }
}

void BM_EtherToStringFormat(benchmark::State& state) {
  for (auto _ : state) {
    auto s = ether_ntos_format(addr);
  }
}

void BM_EtherToStreamCpp(benchmark::State& state) {
  NullBuffer null_buffer;
  std::ostream null_stream(&null_buffer);

  for (auto _ : state) {
    ether_stream_cpp(null_stream, addr);
  }
}

void BM_EtherToStringMove(benchmark::State& state) {
  for (auto _ : state) {
    auto s = ether_ntos_move(addr);
  }
}

void BM_EtherToStringReturn(benchmark::State& state) {
  for (auto _ : state) {
    auto s = ether_ntos_return(addr);
  }
}

void BM_SockAddrToStreamFormat(benchmark::State& state) {
  NullBuffer null_buffer;
  std::ostream null_stream(&null_buffer);

  for (auto _ : state) {
    sockaddr_stream_format(null_stream, ipaddr);
  }
}

void BM_SockAddrToStringFormat(benchmark::State& state) {
  for (auto _ : state) {
    auto s = inet_ntos_format(ipaddr);
  }
}

void BM_SockAddrToStreamCpp(benchmark::State& state) {
  NullBuffer null_buffer;
  std::ostream null_stream(&null_buffer);

  for (auto _ : state) {
    sockaddr_stream_cpp(null_stream, ipaddr);
  }
}

void BM_SockAddrToStringMove(benchmark::State& state) {
  for (auto _ : state) {
    auto s = inet_ntos_move(ipaddr);
  }
}

void BM_SockAddrToStringReturn(benchmark::State& state) {
  for (auto _ : state) {
    auto s = inet_ntos_return(ipaddr);
  }
}

}  // namespace

// Register the function as a benchmark
BENCHMARK(BM_EtherToStreamFormat);
BENCHMARK(BM_EtherToStringFormat);
BENCHMARK(BM_EtherToStreamCpp);
BENCHMARK(BM_EtherToStringMove);
BENCHMARK(BM_EtherToStringReturn);
BENCHMARK(BM_SockAddrToStreamFormat);
BENCHMARK(BM_SockAddrToStringFormat);
BENCHMARK(BM_SockAddrToStreamCpp);
BENCHMARK(BM_SockAddrToStringMove);
BENCHMARK(BM_SockAddrToStringReturn);

// Run the benchmark
BENCHMARK_MAIN();

// NOLINTEND
