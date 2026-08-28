// Bounded SPSC ring buffer
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "tmp.hpp"

namespace syslib {

namespace detail {
inline constexpr std::size_t next_pow2(std::size_t v) {
  if (v <= 1) return 1;
  --v;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
#if SIZE_MAX > UINT32_MAX
  v |= v >> 32;
#endif
  return v + 1;
}

inline std::size_t checked_capacity(std::size_t requested) {
  if (requested == 0) throw std::invalid_argument("spsc_ring capacity must be greater than zero");
  const std::size_t capacity = next_pow2(requested);
  if (capacity == 0 || capacity < requested) throw std::length_error("spsc_ring capacity is too large");
  return capacity;
}
}

template <class T, class Policy = queue_policy<queue_kind::spsc>>
class spsc_ring {
 public:
  using value_type = T;
  using policy_type = Policy;

  explicit spsc_ring(std::size_t capacity)
      : capacity_(detail::checked_capacity(capacity)), mask_(capacity_ - 1),
        storage_(capacity_) {}

  spsc_ring(const spsc_ring&) = delete;
  spsc_ring& operator=(const spsc_ring&) = delete;

  // Producer side
  bool try_push(const T& v) noexcept(std::is_nothrow_copy_constructible_v<T>) {
    auto head = head_.load(std::memory_order_relaxed);
    if (head - tail_.load(std::memory_order_acquire) >= capacity_) return false;
    storage_[head & mask_].emplace(v);
    head_.store(head + 1, policy_type::write_order);
    return true;
  }

  bool try_push(T&& v) noexcept(std::is_nothrow_move_constructible_v<T>) {
    auto head = head_.load(std::memory_order_relaxed);
    if (head - tail_.load(std::memory_order_acquire) >= capacity_) return false;
    storage_[head & mask_].emplace(std::move(v));
    head_.store(head + 1, policy_type::write_order);
    return true;
  }

  template <class... Args>
  bool try_emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    auto head = head_.load(std::memory_order_relaxed);
    if (head - tail_.load(std::memory_order_acquire) >= capacity_) return false;
    storage_[head & mask_].emplace(std::forward<Args>(args)...);
    head_.store(head + 1, policy_type::write_order);
    return true;
  }

  void push_wait(const T& v) {
    typename policy_type::backoff bk{};
    while (!try_push(v)) bk();
  }

  void push_wait(T&& v) {
    typename policy_type::backoff bk{};
    while (!try_push(std::move(v))) bk();
  }

  // Consumer side
  bool try_pop(T& out) noexcept(std::is_nothrow_move_assignable_v<T>) {
    auto tail = tail_.load(std::memory_order_relaxed);
    if (tail == head_.load(policy_type::read_order)) return false;
    auto& slot = storage_[tail & mask_];
    out = std::move(*slot);
    slot.reset();
    tail_.store(tail + 1, std::memory_order_release);
    return true;
  }

  [[nodiscard]] std::optional<T> try_pop() {
    auto tail = tail_.load(std::memory_order_relaxed);
    if (tail == head_.load(policy_type::read_order)) return std::nullopt;
    auto& slot = storage_[tail & mask_];
    std::optional<T> result(std::in_place, std::move(*slot));
    slot.reset();
    tail_.store(tail + 1, std::memory_order_release);
    return result;
  }

  T pop_wait() {
    typename policy_type::backoff bk{};
    for (;;) {
      auto value = try_pop();
      if (value.has_value()) return std::move(*value);
      bk();
    }
  }

  [[nodiscard]] bool empty() const noexcept {
    return size() == 0;
  }

  [[nodiscard]] std::size_t size() const noexcept {
    auto h = head_.load(std::memory_order_acquire);
    auto t = tail_.load(std::memory_order_acquire);
    return h - t;
  }

  [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

 private:
  const std::size_t capacity_;
  const std::size_t mask_;
  alignas(64) std::atomic<std::size_t> head_{0};
  alignas(64) std::atomic<std::size_t> tail_{0};
  std::vector<std::optional<T>> storage_;
};

// Trait: this implementation is lock-free (uses lock-free atomics if available)
template <class T, class P>
struct is_lock_free<spsc_ring<T, P>> : std::bool_constant<std::atomic<std::size_t>::is_always_lock_free> {};

}  // namespace syslib
