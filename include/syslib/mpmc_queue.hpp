// Correct, bounded-memory multiple-producer/multiple-consumer queue.
#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

#include "tmp.hpp"

namespace syslib {

// The queue is unbounded in capacity, but its storage is proportional to the
// number of queued values. Dequeued values are reclaimed immediately. The
// implementation uses a mutex because the previous Michael-Scott scaffold had
// no safe runtime reclamation strategy.
template <class T, class Policy = queue_policy<queue_kind::mpmc>>
class mpmc_queue {
 public:
  using value_type = T;
  using policy_type = Policy;

  mpmc_queue() = default;
  mpmc_queue(const mpmc_queue&) = delete;
  mpmc_queue& operator=(const mpmc_queue&) = delete;
  mpmc_queue(mpmc_queue&&) = delete;
  mpmc_queue& operator=(mpmc_queue&&) = delete;

  void enqueue(const T& value) { emplace(value); }
  void enqueue(T&& value) { emplace(std::move(value)); }

  template <class... Args>
  void emplace(Args&&... args) {
    {
      std::lock_guard lock(mutex_);
      queue_.emplace_back(std::forward<Args>(args)...);
    }
    available_.notify_one();
  }

  bool try_dequeue(T& out) {
    std::lock_guard lock(mutex_);
    if (queue_.empty()) return false;
    out = std::move(queue_.front());
    queue_.pop_front();
    return true;
  }

  [[nodiscard]] std::optional<T> try_dequeue() {
    std::lock_guard lock(mutex_);
    if (queue_.empty()) return std::nullopt;
    std::optional<T> result(std::in_place, std::move(queue_.front()));
    queue_.pop_front();
    return result;
  }

  void push_wait(const T& value) { enqueue(value); }
  void push_wait(T&& value) { enqueue(std::move(value)); }

  T pop_wait() {
    std::unique_lock lock(mutex_);
    available_.wait(lock, [this] { return !queue_.empty(); });
    try {
      T result(std::move(queue_.front()));
      queue_.pop_front();
      return result;
    } catch (...) {
      const bool value_available = !queue_.empty();
      lock.unlock();
      if (value_available) available_.notify_one();
      throw;
    }
  }

  [[nodiscard]] bool empty() const {
    std::lock_guard lock(mutex_);
    return queue_.empty();
  }

  [[nodiscard]] std::size_t size() const {
    std::lock_guard lock(mutex_);
    return queue_.size();
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable available_;
  std::deque<T> queue_;
};

template <class T, class P>
struct is_lock_free<mpmc_queue<T, P>> : std::false_type {};

}  // namespace syslib
