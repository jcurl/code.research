#include "core_benchmark.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <thread>

#include "sync_event.h"
#include "thread_pin.h"

namespace {
template <std::uint32_t compare, std::uint32_t swap>
__attribute__((always_inline)) inline auto cas(std::atomic<std::uint32_t> &flag)
    -> void {
#if __x86_64__
  asm volatile(
      " mov %1,%%esi;"
      "0:"
      " mov %0,%%eax;"
      " lock cmpxchg %%esi,%2;"
      " jne 0b;"
      :
      : "i"(compare), "i"(swap), "m"(flag)
      : "eax", "esi");
#elif __i386__
  asm volatile(
      " mov %1,%%ebx;"
      "0:"
      " mov %0,%%eax;"
      " lock cmpxchg %%ebx,%2;"
      " jne 0b;"
      :
      : "i"(compare), "i"(swap), "m"(flag)
      : "eax", "ebx");
#else
  std::uint32_t expected_flag = compare;
  do {
    expected_flag = compare;
  } while (!flag.compare_exchange_strong(
      expected_flag, swap, std::memory_order::memory_order_relaxed,
      std::memory_order::memory_order_relaxed));
#endif
}
}  // namespace

auto core_benchmark::name() const -> std::string { return std::string{"CAS"}; }

auto core_benchmark::init() -> void { this->flag_ = PING; }

auto core_benchmark::run(std::uint32_t ping_core, std::uint32_t pong_core)
    -> std::uint32_t {
  if (ping_core >= std::thread::hardware_concurrency() ||
      pong_core >= std::thread::hardware_concurrency() ||
      ping_core == pong_core) {
    std::abort();
  }
  sync_event flag{};

  std::thread ping_thread([&]() {
    thread_pin_core(ping_core);
    flag.wait();

    for (std::uint32_t i = 0; i < iterations_ * samples_; i++) {
      cas<PING, PONG>(this->flag_);
    }
  });

  std::uint64_t total_time = 0;
  std::thread pong_thread([&]() {
    thread_pin_core(pong_core);
    flag.wait();

    for (std::uint32_t i = 0; i < samples_; i++) {
      auto start = std::chrono::high_resolution_clock::now();
      for (std::uint32_t j = 0; j < iterations_; j++) {
        cas<PONG, PING>(this->flag_);
      }
      auto end = std::chrono::high_resolution_clock::now();
      std::uint32_t duration = std::chrono::nanoseconds(end - start).count();
      total_time += duration;
    }
  });

  flag.set();

  ping_thread.join();
  pong_thread.join();
  return total_time / iterations_ / samples_ / 2;
}
