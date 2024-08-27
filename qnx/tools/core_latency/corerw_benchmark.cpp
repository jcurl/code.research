#include "corerw_benchmark.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "sync_event.h"
#include "thread_pin.h"

auto corerw_benchmark::name() const -> std::string {
  return std::string{"Read/Write"};
}

auto corerw_benchmark::init() -> void {
  // Both must be PING, else there may be race conditions.
  this->ping_ = PING;
  this->pong_ = PING;
}

auto corerw_benchmark::run(std::uint32_t ping_core, std::uint32_t pong_core)
    -> std::uint32_t {
  if (ping_core >= std::thread::hardware_concurrency() ||
      pong_core >= std::thread::hardware_concurrency() ||
      ping_core == pong_core) {
    std::abort();
  }
  sync_event flag{};

  std::thread pong_thread([&]() {
    thread_pin_core(pong_core);
    flag.wait();

    std::uint32_t v = PING;
    for (std::uint32_t i = 0; i < iterations_ * samples_; i++) {
      while (this->ping_.load(std::memory_order::memory_order_acquire) != v) {
      }
      this->pong_.store(!v, std::memory_order::memory_order_release);
      v = !v;
    }
  });

  std::uint64_t total_time = 0;
  std::thread ping_thread([&]() {
    thread_pin_core(ping_core);
    flag.wait();

    std::uint32_t v = PONG;
    for (std::uint32_t i = 0; i < samples_; i++) {
      auto start = std::chrono::high_resolution_clock::now();
      for (std::uint32_t j = 0; j < iterations_; j++) {
        while (this->pong_.load(std::memory_order::memory_order_acquire) != v) {
        }
        this->ping_.store(v, std::memory_order::memory_order_release);
        v = !v;
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
