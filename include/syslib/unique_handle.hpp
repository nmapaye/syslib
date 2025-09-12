// unique_handle: generalized RAII wrapper for OS handles
#pragma once

#include <utility>
#include <type_traits>

namespace syslib {

template <class Handle, Handle Invalid, class Deleter>
class unique_handle {
 public:
  using handle_type = Handle;
  using deleter_type = Deleter;

  constexpr unique_handle() noexcept = default;
  explicit unique_handle(handle_type h) noexcept : h_(h) {}
  unique_handle(handle_type h, const deleter_type& d) noexcept : h_(h), d_(d) {}

  unique_handle(const unique_handle&) = delete;
  unique_handle& operator=(const unique_handle&) = delete;

  unique_handle(unique_handle&& other) noexcept(std::is_nothrow_move_constructible_v<deleter_type>)
      : h_(other.release()), d_(std::move(other.d_)) {}

  unique_handle& operator=(unique_handle&& other) noexcept(
      std::is_nothrow_move_assignable_v<deleter_type>) {
    if (this != &other) {
      reset();
      d_ = std::move(other.d_);
      h_ = other.release();
    }
    return *this;
  }

  ~unique_handle() { reset(); }

  void reset(handle_type h = Invalid) noexcept {
    if (h_ != Invalid) d_(h_);
    h_ = h;
  }

  [[nodiscard]] handle_type get() const noexcept { return h_; }

  [[nodiscard]] handle_type release() noexcept {
    handle_type tmp = h_;
    h_ = Invalid;
    return tmp;
  }

  [[nodiscard]] explicit operator bool() const noexcept { return h_ != Invalid; }

  [[nodiscard]] deleter_type& get_deleter() noexcept { return d_; }
  [[nodiscard]] const deleter_type& get_deleter() const noexcept { return d_; }

  static constexpr handle_type invalid() noexcept { return Invalid; }

 private:
  handle_type h_ = Invalid;
  deleter_type d_{};
};

}  // namespace syslib

