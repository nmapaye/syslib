// Lightweight scope guard for RAII cleanup
#pragma once

#include <utility>

namespace syslib {

template <class F>
class scope_guard {
 public:
  explicit scope_guard(F&& fn) noexcept(std::is_nothrow_move_constructible_v<F>)
      : fn_(std::forward<F>(fn)), active_(true) {}

  scope_guard(scope_guard&& other) noexcept(std::is_nothrow_move_constructible_v<F>)
      : fn_(std::move(other.fn_)), active_(other.active_) {
    other.active_ = false;
  }

  scope_guard(const scope_guard&) = delete;
  scope_guard& operator=(const scope_guard&) = delete;

  ~scope_guard() noexcept(noexcept(std::declval<F&>()())) {
    if (active_) fn_();
  }

  void release() noexcept { active_ = false; }

 private:
  F fn_;
  bool active_;
};

template <class F>
[[nodiscard]] inline scope_guard<F> make_scope_guard(F&& f) {
  return scope_guard<F>(std::forward<F>(f));
}

}  // namespace syslib

