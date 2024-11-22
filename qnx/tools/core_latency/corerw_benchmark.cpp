#include "corerw_benchmark.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "ubench/thread.h"
#include "statistics.h"

auto corerw_benchmark::name() const -> std::string {
  return std::string{"Read/Write"};
}

auto corerw_benchmark::init() -> bool {
  // Both must be PING, else there may be race conditions.
  this->ping_ = PING;
  this->pong_ = PING;
  return true;
}

auto corerw_benchmark::run(std::uint32_t ping_core, std::uint32_t pong_core)
    -> std::uint32_t {
  if (ping_core >= std::thread::hardware_concurrency() ||
      pong_core >= std::thread::hardware_concurrency() ||
      ping_core == pong_core) {
    std::abort();
  }
  ubench::thread::sync_event flag{};

  std::thread pong_thread([&]() {
    if (!ubench::thread::pin_core(pong_core)) {
      perror("Could not pin 'pong' core");
      std::abort();
    }
    flag.wait();

    std::uint32_t v = PING;
    for (std::uint32_t i = 0; i < iterations_ * samples_; i++) {
      while (this->ping_.load(std::memory_order_acquire) != v) {
      }
      this->pong_.store(!v, std::memory_order_release);
      v = !v;
    }
  });

  statistics stats{};
  std::thread ping_thread([&]() {
    if (!ubench::thread::pin_core(ping_core)) {
      perror("Could not pin 'ping' core");
      std::abort();
    }
    flag.wait();

    std::uint32_t v = PONG;
    for (std::uint32_t i = 0; i < samples_; i++) {
      auto start = std::chrono::high_resolution_clock::now();
      for (std::uint32_t j = 0; j < iterations_; j++) {
        while (this->pong_.load(std::memory_order_acquire) != v) {
        }
        this->ping_.store(v, std::memory_order_release);
        v = !v;
      }
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
