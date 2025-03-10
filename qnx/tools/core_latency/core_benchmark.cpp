#include "config.h"

#include "core_benchmark.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#include "ubench/string.h"
#include "ubench/thread.h"
#include "arm64.h"
#include "statistics.h"

namespace {

static const std::unordered_map<std::string_view, cas_type> supported_ = {
    {"default", cas_type::cpp},
#if defined(i386) || defined(__i386__) || defined(__i386)
    {"cas", cas_type::x86},
#endif
#if defined(__x86_64__)
    {"cas", cas_type::x86_64},
#endif
#if defined(__aarch64__)
    {"llsc", cas_type::arm64},
#endif
#if defined(__aarch64__) && HAVE_CXX_ARM64_LSE
    {"lse", cas_type::arm64_lse},
#endif
};

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define UNUSED(expr) \
  do {               \
    (void)(expr);    \
  } while (0)

namespace {
// Compares the value of `compare` with `&flag`.
// loop:
//   if &flag == compare
//     &flag = swap.
//   else
//     goto loop

template <std::uint32_t compare, std::uint32_t swap>
auto cas_default(std::size_t iter, std::atomic<std::uint32_t> &flag) -> void {
  for (std::size_t i = 0; i < iter; i++) {
    std::uint32_t expected_flag = compare;
    do {
      expected_flag = compare;
    } while (!flag.compare_exchange_strong(expected_flag, swap,
        std::memory_order::memory_order_relaxed,
        std::memory_order::memory_order_relaxed));
  }
}

template <std::uint32_t compare, std::uint32_t swap>
auto cas_x86(std::size_t iter, std::atomic<std::uint32_t> &flag) -> void {
#if defined(i386) || defined(__i386__) || defined(__i386)
  for (std::size_t i = 0; i < iter; i++) {
    asm volatile(
        " mov %1,%%ebx;"
        "0:"
        " mov %0,%%eax;"
        " lock cmpxchg %%ebx,%2;"
        " jne 0b;"
        :
        : "i"(compare), "i"(swap), "m"(flag)
        : "eax", "ebx");
  }
#else
  UNUSED(iter);
  UNUSED(flag);
  std::abort();
#endif
}

template <std::uint32_t compare, std::uint32_t swap>
auto cas_x86_64(std::size_t iter, std::atomic<std::uint32_t> &flag) -> void {
#if defined(__x86_64__)
  for (std::size_t i = 0; i < iter; i++) {
    asm volatile(
        " mov %1,%%esi;"
        "0:"
        " mov %0,%%eax;"
        " lock cmpxchg %%esi,%2;"
        " jne 0b;"
        :
        : "i"(compare), "i"(swap), "m"(flag)
        : "eax", "esi");
  }
#else
  UNUSED(iter);
  UNUSED(flag);
  std::abort();
#endif
}

template <std::uint32_t compare, std::uint32_t swap>
auto cas_arm64(std::size_t iter, std::atomic<std::uint32_t> &flag) -> void {
#if defined(__aarch64__)
  for (std::size_t i = 0; i < iter; i++) {
    asm volatile(
        "0:"
        " mov w1, %1;"
        "1:"
        " ldxr w0, %2;"
        " cmp w0, %0;"
        " b.ne 1b;"
        " stxr w17, w1, %2;"
        " cbnz w17, 1b;"
        :
        : "i"(compare), "i"(swap), "m"(flag)
        : "w0", "w1", "w17");
  }
#else
  UNUSED(iter);
  UNUSED(flag);
  std::abort();
#endif
}

template <std::uint32_t compare, std::uint32_t swap>
auto cas_arm64lse(std::size_t iter, std::atomic<std::uint32_t> &flag) -> void {
#if defined(__aarch64__) && HAVE_CXX_ARM64_LSE
  for (std::size_t i = 0; i < iter; i++) {
    asm volatile(
        "  mov w1, %1;"
        "0:"
        "  mov w0, %0;"
        "  cas w0, w1, %2;"
        "  cmp w0, %0;"
        "  b.ne 0b;"
        :
        : "i"(compare), "i"(swap), "m"(flag)
        : "w0", "w1");
  }
#else
  UNUSED(iter);
  UNUSED(flag);
  std::abort();
#endif
}

template <std::uint32_t compare, std::uint32_t swap>
auto cas(cas_type ctype, std::size_t iter, std::atomic<std::uint32_t> &flag)
    -> void {
  switch (ctype) {
    case cas_type::cpp:
      cas_default<compare, swap>(iter, flag);
      break;
    case cas_type::x86:
      cas_x86<compare, swap>(iter, flag);
      break;
    case cas_type::x86_64:
      cas_x86_64<compare, swap>(iter, flag);
      break;
    case cas_type::arm64:
      cas_arm64<compare, swap>(iter, flag);
      break;
    case cas_type::arm64_lse:
      cas_arm64lse<compare, swap>(iter, flag);
      break;
    default:
      std::abort();
  }
}

}  // namespace

auto core_benchmark::supported() noexcept
    -> const std::unordered_map<std::string_view, cas_type> & {
  return supported_;
}

auto core_benchmark::mode(std::string_view benchmark) noexcept
    -> std::optional<cas_type> {
  auto it = core_benchmark::supported().find(benchmark);
  if (it == core_benchmark::supported().end()) return {};

  return it->second;
}

auto core_benchmark::name() const -> std::string {
  switch (ctype_) {
    case cas_type::cpp:
      return {"CAS (CPP)"};
    case cas_type::x86:
      return {"CAS_x86"};
    case cas_type::x86_64:
      return {"CAS_x86_64"};
    case cas_type::arm64:
      return {"CAS_arm64"};
    case cas_type::arm64_lse:
      return {"CAS_arm64_lse"};
    default:
      std::abort();
  }
}

auto core_benchmark::init() -> bool {
  flag_ = PING;

  if (ctype_ == cas_type::arm64_lse && !has_arm64_lse()) return false;
  return true;
}

auto core_benchmark::run(std::uint32_t ping_core, std::uint32_t pong_core)
    -> std::uint32_t {
  if (ping_core >= std::thread::hardware_concurrency() ||
      pong_core >= std::thread::hardware_concurrency() ||
      ping_core == pong_core) {
    std::abort();
  }

  statistics stats{};
  ubench::thread::sync_event flag{};

  std::thread ping_thread([&]() {
    auto success = ubench::thread::pin_core(ping_core);
    if (!success) {
      std::cerr << "Could not pin 'ping' core; "
                << ubench::string::perror(success.error()) << std::endl;
      std::abort();
    }
    flag.wait();

    std::size_t l = static_cast<std::size_t>(iterations_) * samples_;
    cas<PING, PONG>(ctype_, l, flag_);
  });

  std::thread pong_thread([&]() {
    auto success = ubench::thread::pin_core(pong_core);
    if (!success) {
      std::cerr << "Could not pin 'pong' core; "
                << ubench::string::perror(success.error()) << std::endl;
      std::abort();
    }
    flag.wait();

    for (std::uint32_t i = 0; i < samples_; i++) {
      auto start = std::chrono::high_resolution_clock::now();
      cas<PONG, PING>(ctype_, iterations_, flag_);
      auto end = std::chrono::high_resolution_clock::now();
      std::uint32_t duration = std::chrono::nanoseconds(end - start).count();
      stats.insert(duration);
    }
  });

  flag.set();
  ping_thread.join();
  pong_thread.join();
  return stats.median() / iterations_ / 2;
}
