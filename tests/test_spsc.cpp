#include <gtest/gtest.h>

#include <thread>
#include <limits>
#include <stdexcept>
#include <vector>

#include "syslib/spsc_ring.hpp"

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

}  // namespace

TEST(SPSC, BasicPushPop) {
  spsc_ring<int> q(8);
  EXPECT_TRUE(q.try_push(1));
  EXPECT_TRUE(q.try_push(2));
  int v = 0;
  EXPECT_TRUE(q.try_pop(v));
  EXPECT_EQ(v, 1);
  EXPECT_TRUE(q.try_pop(v));
  EXPECT_EQ(v, 2);
}

TEST(SPSC, WrapAround) {
  spsc_ring<int> q(4);
  for (int i = 0; i < 16; ++i) {
    ASSERT_TRUE(q.try_push(i));
    int v;
    ASSERT_TRUE(q.try_pop(v));
    ASSERT_EQ(v, i);
  }
}

TEST(SPSC, ProducerConsumerThreads) {
  constexpr int N = 1000;
  spsc_ring<int> q(1024);
  std::atomic<int> sum{0};

  std::thread prod([&]{
    for (int i = 1; i <= N; ++i) q.push_wait(i);
  });
  std::thread cons([&]{
    for (int i = 0; i < N; ++i) sum.fetch_add(q.pop_wait(), std::memory_order_relaxed);
  });
  prod.join();
  cons.join();
  EXPECT_EQ(sum.load(), (N * (N + 1)) / 2);
}

TEST(SPSC, SupportsMoveOnlyNonDefaultConstructibleValues) {
  spsc_ring<MoveOnly> queue(3);
  EXPECT_EQ(queue.capacity(), 4U);
  ASSERT_TRUE(queue.try_emplace(9));
  auto value = queue.try_pop();
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(value->value, 9);
  queue.try_emplace(11);
  EXPECT_EQ(queue.pop_wait().value, 11);
}

TEST(SPSC, RejectsInvalidCapacity) {
  EXPECT_THROW((spsc_ring<int>(0)), std::invalid_argument);
  EXPECT_THROW((spsc_ring<int>(std::numeric_limits<std::size_t>::max())), std::length_error);
}
