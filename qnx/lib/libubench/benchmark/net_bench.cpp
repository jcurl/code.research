#include <array>
#include <iomanip>
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
// Unable to determine clock rate from sysctl: machdep.tsc_freq: No such file or
// directory This does not affect benchmark measurements, only the metadata
// output.
// ***WARNING*** Failed to set thread affinity. Estimated CPU frequency may be
// incorrect. 2025-01-18T13:10:26+01:00 Running ./net_bench Run on (4 X 54.0003
// MHz CPU s) Load Average: 0.00, 0.00, 0.00
// --------------------------------------------------------------------
// Benchmark                          Time             CPU   Iterations
// --------------------------------------------------------------------
// BM_EtherToStreamFormat          3472 ns         3471 ns       200789
// BM_EtherToStringFormat          2934 ns         2934 ns       238985
// BM_EtherToStreamCpp             3157 ns         3157 ns       221704
// BM_EtherToStringMove            7130 ns         7129 ns        98082
// BM_EtherToStringReturn          7133 ns         7132 ns        97591
// BM_SockAddrToStreamFormat       3481 ns         3480 ns       201441
// BM_SockAddrToStringFormat       2967 ns         2966 ns       236961
// BM_SockAddrToStreamCpp          2547 ns         2547 ns       274445
// BM_SockAddrToStringMove         5604 ns         5603 ns       117317
// BM_SockAddrToStringReturn       5343 ns         5343 ns       132915

// BM_EtherToStreamFormat     3471 ns // snprintf(ether_addr) to string,
//                                       then stream the string to ostream.
// BM_EtherToStringFormat     2934 ns // snprintf(ether_addr) to string.
// BM_EtherToStreamCpp        3157 ns // stream directly to ostream.
// BM_EtherToStringMove       7129 ns // stream to stringstream, move data
//                                       to string.
// BM_EtherToStringReturn     7132 ns // stream to stringstream, return
//                                       underlying string.

// Results
// - Choose a trade-off.
//   - Put the formatting in the `<<` operator:
//     - The `<<` operator is faster, but then returning the underlying string,
//       which use `stringstream` are ~ 2x slower than the stream. Note, this
//       doesn't take into account the fact that a stringstream can't reserve
//       space upfront, so may be slower due to unexpected memory allocations.
//       We don't have that problem with strings.
//   - Put the formatting in the function:
//     - The `<<` is slower, we create the string (via `std::snprintf`), then
//       write the string to the stream.
//
// While streaming directly to an ostream is slightly faster, if the user wants
// the string, it's a significant impact. Decide to use the `snprintf` solution.

namespace {

auto ether_ntos_format(const ubench::net::ether_addr& addr) -> std::string {
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

auto ether_ntos_move(const ubench::net::ether_addr& addr) -> std::string {
  std::stringstream result;
  ether_stream_cpp(result, addr);
  return std::move(*result.rdbuf()).str();
}

auto ether_ntos_return(const ubench::net::ether_addr& addr) -> std::string {
  std::stringstream result;
  ether_stream_cpp(result, addr);
  return result.str();
}

auto inet_ntos_format(const sockaddr_in& addr) -> std::string {
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

auto inet_ntos_move(const sockaddr_in& addr) -> std::string {
  std::stringstream result;
  sockaddr_stream_cpp(result, addr);
  return std::move(*result.rdbuf()).str();
}

auto inet_ntos_return(const sockaddr_in& addr) -> std::string {
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
