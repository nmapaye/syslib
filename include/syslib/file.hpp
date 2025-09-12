// POSIX fd / FILE* RAII helpers
#pragma once

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#include <string>
#include <system_error>

#include "unique_handle.hpp"

namespace syslib {

struct fd_closer {
  void operator()(int fd) const noexcept {
    if (fd >= 0) ::close(fd);
  }
};

using unique_fd = unique_handle<int, -1, fd_closer>;

inline unique_fd open_fd(const char* path, int flags, mode_t mode = 0644) {
  int fd = ::open(path, flags, mode);
  if (fd < 0) throw std::system_error(errno, std::generic_category(), "open");
  return unique_fd(fd);
}

struct file_closer {
  void operator()(std::FILE* f) const noexcept {
    if (f) std::fclose(f);
  }
};

using unique_file = unique_handle<std::FILE*, static_cast<std::FILE*>(nullptr), file_closer>;

inline unique_file fopen_file(const char* path, const char* mode) {
  std::FILE* f = std::fopen(path, mode);
  if (!f) throw std::system_error(errno, std::generic_category(), "fopen");
  return unique_file(f);
}

}  // namespace syslib
