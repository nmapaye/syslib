#include <benchmark/benchmark.h>

#include <thread>

#include "syslib/spsc_ring.hpp"

using namespace syslib;

static void BM_SPSC_PushPop(benchmark::State& state) {
  spsc_ring<int> q(1u << 16);
  int sum = 0;
  for (auto _ : state) {
    for (int i = 0; i < 1000; ++i) {
      q.push_wait(i);
      sum += q.pop_wait();
    }
  }
  benchmark::DoNotOptimize(sum);
}
BENCHMARK(BM_SPSC_PushPop);

static void BM_SPSC_1P1C(benchmark::State& state) {
  spsc_ring<int> q(1u << 16);
  for (auto _ : state) {
    std::atomic<bool> go{false};
    std::thread prod([&]{
      while (!go.load(std::memory_order_acquire)) {}
      for (int i = 0; i < 100000; ++i) q.push_wait(1);
    });
    std::thread cons([&]{
      go.store(true, std::memory_order_release);
      int c = 0; while (c < 100000) c += q.pop_wait();
    });
    prod.join();
    cons.join();
  }
}
BENCHMARK(BM_SPSC_1P1C)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();

