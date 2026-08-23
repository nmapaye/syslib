#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include "syslib/mpmc_queue.hpp"

using namespace std::chrono_literals;
using namespace syslib;

namespace {

struct MoveOnly {
  explicit MoveOnly(int value) : value(value) {}
  MoveOnly() = delete;
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) noexcept = default;
  MoveOnly& operator=(MoveOnly&&) noexcept = default;
  int value;
};

struct LifetimeTracked {
  explicit LifetimeTracked(int value) : value(value) { ++live; }
  LifetimeTracked(const LifetimeTracked&) = delete;
  LifetimeTracked& operator=(const LifetimeTracked&) = delete;
  LifetimeTracked(LifetimeTracked&& other) noexcept : value(other.value) { ++live; }
  LifetimeTracked& operator=(LifetimeTracked&& other) noexcept {
    value = other.value;
    return *this;
  }
  ~LifetimeTracked() { --live; }
  int value;
  static inline std::atomic<int> live{0};
};

}  // namespace

TEST(MPMC, BasicEnqueueDequeueAndInspection) {
  mpmc_queue<int> queue;
  EXPECT_TRUE(queue.empty());
  queue.enqueue(123);
  EXPECT_EQ(queue.size(), 1U);
  auto value = queue.try_dequeue();
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, 123);
  EXPECT_TRUE(queue.empty());
}

TEST(MPMC, SupportsMoveOnlyNonDefaultConstructibleValues) {
  mpmc_queue<MoveOnly> queue;
  queue.emplace(42);
  auto value = queue.try_dequeue();
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(value->value, 42);
  EXPECT_FALSE(queue.try_dequeue().has_value());
}

TEST(MPMC, BlockingPopWakesForAProducer) {
  mpmc_queue<MoveOnly> queue;
  std::promise<int> result;
  auto ready = result.get_future();
  std::thread consumer([&] { result.set_value(queue.pop_wait().value); });
  EXPECT_EQ(ready.wait_for(20ms), std::future_status::timeout);
  queue.emplace(77);
  EXPECT_EQ(ready.wait_for(1s), std::future_status::ready);
  EXPECT_EQ(ready.get(), 77);
  consumer.join();
}

TEST(MPMC, ReclaimsDequeuedStorageImmediately) {
  EXPECT_EQ(LifetimeTracked::live.load(), 0);
  mpmc_queue<LifetimeTracked> queue;
  queue.emplace(5);
  EXPECT_EQ(LifetimeTracked::live.load(), 1);
  auto value = queue.try_dequeue();
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(LifetimeTracked::live.load(), 1);
  value.reset();
  EXPECT_EQ(LifetimeTracked::live.load(), 0);
}

TEST(MPMC, FourProducersAndConsumersDeliverEveryValueExactlyOnce) {
  constexpr int producer_count = 4;
  constexpr int consumer_count = 4;
  constexpr int values_per_producer = 2000;
  constexpr int total = producer_count * values_per_producer;
  mpmc_queue<int> queue;
  std::vector<std::atomic<int>> seen(total);
  std::atomic<int> consumed{0};
  std::vector<std::thread> threads;

  for (int producer = 0; producer < producer_count; ++producer) {
    threads.emplace_back([&, producer] {
      const int begin = producer * values_per_producer;
      for (int value = begin; value < begin + values_per_producer; ++value) queue.enqueue(value);
    });
  }
  for (int consumer = 0; consumer < consumer_count; ++consumer) {
    threads.emplace_back([&] {
      for (;;) {
        auto value = queue.try_dequeue();
        if (value.has_value()) {
          seen[*value].fetch_add(1, std::memory_order_relaxed);
          if (consumed.fetch_add(1, std::memory_order_relaxed) + 1 == total) return;
        } else if (consumed.load(std::memory_order_relaxed) >= total) {
          return;
        } else {
          std::this_thread::yield();
        }
      }
    });
  }
  for (auto& thread : threads) thread.join();

  EXPECT_EQ(consumed.load(), total);
  EXPECT_TRUE(queue.empty());
  for (const auto& count : seen) EXPECT_EQ(count.load(), 1);
}
