#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "syslib/mpmc_queue.hpp"

using namespace syslib;

TEST(MPMC, BasicEnqDeq) {
  mpmc_queue<int> q;
  q.enqueue(123);
  int v = 0;
  ASSERT_TRUE(q.try_dequeue(v));
  EXPECT_EQ(v, 123);
}

TEST(MPMC, MultiProducerMultiConsumer) {
  mpmc_queue<int> q;
  constexpr int P = 2;
  constexpr int C = 2;
  constexpr int N = 1000;

  std::atomic<int> produced{0};
  std::atomic<int> consumed{0};

  std::vector<std::thread> threads;
  for (int p = 0; p < P; ++p) {
    threads.emplace_back([&]{
      for (int i = 0; i < N; ++i) {
        q.enqueue(1);
        produced.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }
  for (int c = 0; c < C; ++c) {
    threads.emplace_back([&]{
      int local = 0;
      while (local < (P*N)/C) {
        int v;
        if (q.try_dequeue(v)) {
          local += v;
          consumed.fetch_add(v, std::memory_order_relaxed);
        } else {
          std::this_thread::yield();
        }
      }
    });
  }
  for (auto& t : threads) t.join();

  EXPECT_EQ(produced.load(), P * N);
  EXPECT_EQ(consumed.load(), P * N);
}

