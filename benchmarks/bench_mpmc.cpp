#include <benchmark/benchmark.h>

#include <atomic>
#include <thread>
#include <vector>

#include "syslib/mpmc_queue.hpp"

using namespace syslib;

static void BM_MPMC_Throughput(benchmark::State& state) {
  const int threads = static_cast<int>(state.range(0));
  const int producers = threads / 2 + (threads % 2);
  const int consumers = threads - producers;
  mpmc_queue<int> q;

  for (auto _ : state) {
    std::atomic<bool> go{false};
    std::vector<std::thread> ts;
    for (int i = 0; i < producers; ++i) {
      ts.emplace_back([&]{
        while (!go.load(std::memory_order_acquire)) {}
        for (int k = 0; k < 50000; ++k) q.enqueue(1);
      });
    }
    for (int i = 0; i < consumers; ++i) {
      ts.emplace_back([&]{
        int local = 0;
        go.store(true, std::memory_order_release);
        while (local < (producers * 50000) / consumers) {
          int v;
          if (q.try_dequeue(v)) local += v; else std::this_thread::yield();
        }
      });
    }
    for (auto& t : ts) t.join();
  }
}

BENCHMARK(BM_MPMC_Throughput)->Arg(2)->Arg(4)->Arg(6)->Arg(8);

BENCHMARK_MAIN();

