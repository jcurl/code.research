#include "core_benchmark.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <thread>

#include "sync_event.h"
#include "thread_pin.h"

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
      std::uint32_t expected_flag = PING;
      do {
        expected_flag = PING;
      } while (!flag_.compare_exchange_strong(
          expected_flag, PONG, std::memory_order::memory_order_relaxed,
          std::memory_order::memory_order_relaxed));
    }
  });

  std::uint64_t total_time = 0;
  std::thread pong_thread([&]() {
    thread_pin_core(pong_core);
    flag.wait();

    for (std::uint32_t i = 0; i < samples_; i++) {
      auto start = std::chrono::high_resolution_clock::now();
      for (std::uint32_t j = 0; j < iterations_; j++) {
        std::uint32_t expected_flag = PONG;
        do {
          expected_flag = PONG;
        } while (!flag_.compare_exchange_strong(
            expected_flag, PING, std::memory_order::memory_order_relaxed,
            std::memory_order::memory_order_relaxed));
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
