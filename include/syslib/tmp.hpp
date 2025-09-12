// TMP policies, traits, and concepts
#pragma once

#include <atomic>
#include <cstddef>
#include <type_traits>
#include <thread>
#include <chrono>

namespace syslib {

// Backoff strategies
namespace backoff {
struct none {
  void operator()() const noexcept {}
};

struct yield {
  void operator()() const noexcept;
};

struct spin_then_yield {
  int spins = 64;
  void operator()() const noexcept;
};
}  // namespace backoff

inline void backoff::yield::operator()() const noexcept { std::this_thread::yield(); }

inline void backoff::spin_then_yield::operator()() const noexcept {
  for (int i = 0; i < spins; ++i) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    __asm__ __volatile__("pause");
#else
    ;
#endif
  }
  std::this_thread::yield();
}

enum class queue_kind { spsc, mpmc };

template <
    queue_kind Kind = queue_kind::spsc,
    std::memory_order WriteOrder = std::memory_order_release,
    std::memory_order ReadOrder = std::memory_order_acquire,
    bool PadCacheLines = true,
    class Backoff = backoff::spin_then_yield>
struct queue_policy {
  static constexpr queue_kind kind = Kind;
  static constexpr std::memory_order write_order = WriteOrder;
  static constexpr std::memory_order read_order = ReadOrder;
  static constexpr bool pad = PadCacheLines;
  using backoff = Backoff;
};

// Cache line padding utility
template <std::size_t N>
struct cacheline_pad {
  alignas(N) std::byte pad_[N]{};
};

// Traits: is_lock_free
template <class T>
struct is_lock_free : std::false_type {};

template <class T>
inline constexpr bool is_lock_free_v = is_lock_free<T>::value;

// Concepts
template <class P>
concept SPSCPolicy = requires {
  { P::kind } -> std::convertible_to<queue_kind>;
  requires P::kind == queue_kind::spsc || P::kind == queue_kind::mpmc;
};

}  // namespace syslib
