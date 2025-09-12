#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>

#include "syslib/spsc_ring.hpp"

using namespace std::chrono;

int main() {
  using Q = syslib::spsc_ring<int>;
  const std::size_t capacity = 1u << 16;
  const int ops = 2'000'000; // 2M operations
  Q q(capacity);

  std::atomic<bool> go{false};
  std::atomic<int> consumed{0};

  std::thread prod([&]{
    while (!go.load(std::memory_order_acquire)) {}
    for (int i = 0; i < ops; ++i) q.push_wait(1);
  });

  auto t_start = steady_clock::now();
  std::thread cons([&]{
    go.store(true, std::memory_order_release);
    int sum = 0;
    for (int i = 0; i < ops; ++i) sum += q.pop_wait();
    consumed.store(sum, std::memory_order_relaxed);
  });

  prod.join();
  cons.join();
  auto t_end = steady_clock::now();
  double secs = duration<double>(t_end - t_start).count();
  double throughput = ops / secs;

  // CSV header: ops_per_sec,ops,duration_s,capacity
  std::printf("%.3f,%d,%.6f,%zu\n", throughput, ops, secs, capacity);
  return 0;
}

