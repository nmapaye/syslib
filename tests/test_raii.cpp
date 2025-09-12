#include <gtest/gtest.h>

#include <cstdio>
#include <string>
#include <unistd.h>

#include "syslib/file.hpp"
#include "syslib/scope_guard.hpp"
#include "syslib/unique_handle.hpp"

using namespace syslib;

TEST(RAII, UniqueHandleBasics) {
  struct dummy_close {
    static inline int called = 0;
    void operator()(int h) const noexcept { if (h != -1) ++called; }
  };

  using uh = unique_handle<int, -1, dummy_close>;
  {
    uh a{42};
    EXPECT_TRUE(a);
  }
  EXPECT_EQ(dummy_close::called, 1);
}

TEST(RAII, FileOpenClose) {
  char tmpl[] = "/tmp/syslib_raii.XXXXXX";
  int fd = mkstemp(tmpl);
  ASSERT_GE(fd, 0);
  ::close(fd);

  auto f = fopen_file(tmpl, "w+");
  ASSERT_TRUE(f);
  const char* msg = "hello";
  std::fwrite(msg, 1, 5, f.get());
}

TEST(RAII, ScopeGuard) {
  int x = 0;
  {
    auto g = make_scope_guard([&]() { x = 42; });
    (void)g;
  }
  EXPECT_EQ(x, 42);
}

