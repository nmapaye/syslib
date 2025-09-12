// POSIX mmap RAII wrapper
#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <system_error>

namespace syslib {

class mapped_region {
 public:
  mapped_region() = default;

  mapped_region(int fd, std::size_t length, off_t offset = 0,
                int prot = PROT_READ | PROT_WRITE,
                int flags = MAP_SHARED) {
    void* p = ::mmap(nullptr, length, prot, flags, fd, offset);
    if (p == MAP_FAILED)
      throw std::system_error(errno, std::generic_category(), "mmap");
    ptr_ = static_cast<std::byte*>(p);
    len_ = length;
  }

  mapped_region(mapped_region&& other) noexcept { move_from(other); }
  mapped_region& operator=(mapped_region&& other) noexcept {
    if (this != &other) {
      unmap();
      move_from(other);
    }
    return *this;
  }

  mapped_region(const mapped_region&) = delete;
  mapped_region& operator=(const mapped_region&) = delete;

  ~mapped_region() { unmap(); }

  [[nodiscard]] void* data() noexcept { return ptr_; }
  [[nodiscard]] const void* data() const noexcept { return ptr_; }
  [[nodiscard]] std::size_t size() const noexcept { return len_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  void advise_sequential() const noexcept {
    if (ptr_) ::madvise(ptr_, len_, MADV_SEQUENTIAL);
  }

 private:
  void unmap() noexcept {
    if (ptr_) ::munmap(ptr_, len_);
    ptr_ = nullptr;
    len_ = 0;
  }

  void move_from(mapped_region& other) noexcept {
    ptr_ = other.ptr_;
    len_ = other.len_;
    other.ptr_ = nullptr;
    other.len_ = 0;
  }

  std::byte* ptr_ = nullptr;
  std::size_t len_ = 0;
};

}  // namespace syslib

