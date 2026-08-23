#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>
#include <algorithm>

#include "syslib/mpmc_queue.hpp"

using namespace std::chrono;

struct Msg {
  uint64_t t0_ns; // timestamp offset from global start
};

static uint64_t now_ns() {
  return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

static void run_case(int producers, int consumers, int msgs_per_producer) {
  syslib::mpmc_queue<Msg> q;
  const int total_msgs = producers * msgs_per_producer;
  std::atomic<bool> go{false};
  std::atomic<int> consumed{0};
  std::vector<std::vector<uint64_t>> lat_samples(consumers);
  const int sample_rate = 1000; // record 1 in 1000 latencies

  // Consumers
  std::vector<std::thread> threads;
  for (int c = 0; c < consumers; ++c) {
    threads.emplace_back([&, c]{
      auto& bucket = lat_samples[c];
      bucket.reserve(msgs_per_producer * producers / consumers / sample_rate + 64);
      while (!go.load(std::memory_order_acquire)) {}
      while (true) {
        Msg m;
        if (q.try_dequeue(m)) {
          int g = consumed.fetch_add(1, std::memory_order_relaxed) + 1;
          if (g % sample_rate == 0) {
            uint64_t t1 = now_ns();
            bucket.push_back(t1 - m.t0_ns);
          }
          if (g >= total_msgs) break;
        } else {
          if (consumed.load(std::memory_order_relaxed) >= total_msgs) break;
          std::this_thread::yield();
        }
      }
    });
  }

  // Producers
  for (int p = 0; p < producers; ++p) {
    threads.emplace_back([&]{
      while (!go.load(std::memory_order_acquire)) {}
      for (int i = 0; i < msgs_per_producer; ++i) {
        Msg m{now_ns()};
        q.enqueue(std::move(m));
      }
    });
  }

  auto t_start = steady_clock::now();
  go.store(true, std::memory_order_release);
  for (auto& t : threads) t.join();
  auto t_end = steady_clock::now();
  double secs = duration<double>(t_end - t_start).count();
  double tput = total_msgs / secs;

  // merge lat samples
  std::vector<uint64_t> all;
  size_t total_samples = 0;
  for (auto& v : lat_samples) total_samples += v.size();
  all.reserve(total_samples);
  for (auto& v : lat_samples) all.insert(all.end(), v.begin(), v.end());
  if (all.empty()) all.push_back(0);
  std::sort(all.begin(), all.end());

  auto pct = [&](double p){
    size_t idx = static_cast<size_t>(p * (all.size() - 1));
    return all[idx];
  };

  uint64_t p50 = pct(0.50);
  uint64_t p90 = pct(0.90);
  uint64_t p99 = pct(0.99);

  // CSV header: producers,consumers,total_msgs,duration_s,throughput_ops_per_sec,p50_ns,p90_ns,p99_ns
  std::printf("%d,%d,%d,%.6f,%.3f,%llu,%llu,%llu\n",
              producers, consumers, total_msgs, secs, tput,
              (unsigned long long)p50,
              (unsigned long long)p90,
              (unsigned long long)p99);
}

int main() {
  // sweep pairs (P=C) from 1..8
  const int msgs_per_producer = 100000; // 100k each
  for (int n = 1; n <= 8; ++n) run_case(n, n, msgs_per_producer);
  return 0;
}
