// Michael-Scott MPMC queue (boilerplate with simple reclamation stub)
#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <type_traits>

#include "tmp.hpp"

namespace syslib {

// A minimal Michael-Scott MPMC queue implementation.
// Note: For simplicity, this boilerplate defers node reclamation until
// queue destruction. This preserves lock-free progress for enq/deq but
// is not suitable for unbounded runtimes (memory grows with operations).
// A production variant should supply hazard-pointer or epoch reclamation.

template <class T, class Policy = queue_policy<queue_kind::mpmc>>
class mpmc_queue {
 private:
  struct node {
    std::atomic<node*> next{nullptr};
    std::optional<T> value;  // nullopt for dummy node
    node* retire_link{nullptr};
    explicit node() : value(std::nullopt) {}
    explicit node(T&& v) : value(std::move(v)) {}
    explicit node(const T& v) : value(v) {}
  };

 public:
  using value_type = T;
  using policy_type = Policy;

  mpmc_queue() {
    node* dummy = new node();
    head_.store(dummy, std::memory_order_relaxed);
    tail_.store(dummy, std::memory_order_relaxed);
    retired_.store(nullptr, std::memory_order_relaxed);
  }

  ~mpmc_queue() {
    // Drain all nodes and free (best-effort, single-threaded context assumed)
    // Delete any nodes still reachable from head
    {
      node* n = head_.load(std::memory_order_relaxed);
      while (n) {
        node* next = n->next.load(std::memory_order_relaxed);
        delete n;
        n = next;
      }
    }
    // Delete retired nodes (already detached from main chain)
    {
      node* r = retired_.load(std::memory_order_relaxed);
      while (r) {
        node* next = r->retire_link;
        delete r;
        r = next;
      }
    }
  }

  mpmc_queue(const mpmc_queue&) = delete;
  mpmc_queue& operator=(const mpmc_queue&) = delete;

  void enqueue(const T& v) { emplace(v); }
  void enqueue(T&& v) { emplace(std::move(v)); }

  template <class... Args>
  void emplace(Args&&... args) {
    node* n = new node(T(std::forward<Args>(args)...));
    n->next.store(nullptr, std::memory_order_relaxed);
    for (;;) {
      node* tail = tail_.load(std::memory_order_acquire);
      node* next = tail->next.load(std::memory_order_acquire);
      if (tail == tail_.load(std::memory_order_acquire)) {
        if (next == nullptr) {
          if (tail->next.compare_exchange_weak(next, n, std::memory_order_release, std::memory_order_relaxed)) {
            // swing tail
            (void)tail_.compare_exchange_strong(tail, n, std::memory_order_release, std::memory_order_relaxed);
            return;
          }
        } else {
          // tail is lagging
          (void)tail_.compare_exchange_strong(tail, next, std::memory_order_release, std::memory_order_relaxed);
        }
      }
    }
  }

  bool try_dequeue(T& out) {
    for (;;) {
      node* head = head_.load(std::memory_order_acquire);
      node* tail = tail_.load(std::memory_order_acquire);
      node* next = head->next.load(std::memory_order_acquire);
      if (head == head_.load(std::memory_order_acquire)) {
        if (next == nullptr) {
          return false;  // empty
        }
        if (head == tail) {
          // tail is lagging
          (void)tail_.compare_exchange_strong(tail, next, std::memory_order_release, std::memory_order_relaxed);
          continue;
        }
        // read value before moving head
        if (next->value.has_value()) out = std::move(*next->value);
        if (head_.compare_exchange_strong(head, next, std::memory_order_release, std::memory_order_relaxed)) {
          // retire old head (dummy)
          retire(head);
          return true;
        }
      }
    }
  }

  void push_wait(const T& v) { enqueue(v); }
  void push_wait(T&& v) { enqueue(std::move(v)); }

  T pop_wait() {
    typename policy_type::backoff bk{};
    T v{};
    while (!try_dequeue(v)) bk();
    return v;
  }

 private:
  // Very simple retire: append to a singly-linked list; freed in destructor.
  void retire(node* n) noexcept {
    // append n to retired list head (LIFO) — not reclaimed until dtor
    node* old = retired_.load(std::memory_order_relaxed);
    do {
      n->retire_link = old;
    } while (!retired_.compare_exchange_weak(old, n, std::memory_order_release, std::memory_order_relaxed));
  }

  alignas(64) std::atomic<node*> head_{nullptr};
  alignas(64) std::atomic<node*> tail_{nullptr};
  // Retired list for deferred destruction (best-effort)
  alignas(64) std::atomic<node*> retired_{nullptr};
};

// Trait: conservatively mark not lock-free due to reclamation stub
template <class T, class P>
struct is_lock_free<mpmc_queue<T, P>> : std::false_type {};

}  // namespace syslib
