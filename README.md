# syslib — Header-only C++20 Systems Library

A small header-only library focused on practical systems building blocks:

- RAII wrappers for POSIX resources (file descriptors, `FILE*`, mmap regions)
- Utilities: `unique_handle`, `scope_guard`, simple sync wrappers
- Concurrent data structures: lock-free bounded SPSC ring buffer and a blocking MPMC queue
- TMP-driven policies: queue kind, memory orders, padding, backoff strategies
- Tooling for tests, benchmarks, sanitizers, and static analysis

## Build

- CMake 3.20+
- C++20 compiler (GCC 11+, Clang 13+, AppleClang 14+)

Option A — Presets (recommended)

```
# Configure
cmake --preset release     # or: debug, asan-ubsan, tsan, bench

# Build
cmake --build --preset release -j

# Test
ctest --preset release
```

Option B — Manual

```
cmake -S . -B build -G Ninja -DSYSLIB_BUILD_TESTS=ON -DSYSLIB_BUILD_BENCHMARKS=ON
cmake --build build -j
ctest --test-dir build -V
```

Tip: Install ccache to speed up incremental builds. Presets already set `CMAKE_CXX_COMPILER_LAUNCHER=ccache` if available.

## Headers & APIs

- `syslib/scope_guard.hpp`: `scope_guard`, `make_scope_guard`
- `syslib/unique_handle.hpp`: `unique_handle<Handle, Invalid, Deleter>`
- `syslib/file.hpp`: `unique_fd`, `open_fd`, `unique_file`, `fopen_file`
- `syslib/mmap.hpp`: `mapped_region`
- `syslib/sync.hpp`: aliases for mutex/locks/condvar
- `syslib/tmp.hpp`: `queue_policy`, backoff strategies, `is_lock_free`
- `syslib/spsc_ring.hpp`: `spsc_ring<T, Policy>`
- `syslib/mpmc_queue.hpp`: `mpmc_queue<T, Policy>` with immediate reclamation and blocking waits
- `syslib/channel.hpp`: `channel<T, Policy>` selects SPSC vs MPMC at compile time

## Template Metaprogramming (TMP)

- `queue_policy<Kind, WriteOrder, ReadOrder, PadCacheLines, Backoff>` chooses queue kind, memory orders, padding, and backoff type at compile time.
- `is_lock_free<T>` trait indicates lock-free properties of the types here.
- `channel<T, Policy>` picks queue implementation with a single name.

## Lock-free Structures

- SPSC ring buffer
  - Bounded, power-of-two capacity, head/tail indices with wrap mask
  - Non-blocking `try_push/try_pop` and blocking `push_wait/pop_wait`
  - Cache-line padding on hot atomics
- MPMC queue
  - Mutex and condition-variable implementation with storage proportional to queued values
  - `try_dequeue` output-parameter and `std::optional<T>` forms
  - `pop_wait` sleeps until data is available instead of spinning
  - Supports move-only, non-default-constructible values

## Tooling

- Tests: GoogleTest (`syslib/tests`)
- Benchmarks: Google Benchmark (`syslib/benchmarks`)
- Sanitizers: enable with `-DSYSLIB_ENABLE_ASAN=ON -DSYSLIB_ENABLE_UBSAN=ON -DSYSLIB_ENABLE_TSAN=ON`
- Static analysis: `.clang-tidy`
- CI: GitHub Actions matrix for Linux/macOS and compilers

## Benchmarks (targets)

Run from the build tree after enabling benchmarks:

```
./bench_spsc
./bench_mpmc
```

Targets to publish and compare by implementation:
- SPSC throughput ≥ 1M ops/sec on an M-class Mac (1P/1C)
- Blocking MPMC scalability across 1 to 8 producers/consumers, including latency percentiles

## Big-O and Memory Model Notes

- SPSC ring
  - `try_push/try_pop`: O(1) amortized; uses `release`/`acquire` for head/tail
  - Avoids false sharing via padding; use policy to adjust
- MPMC queue
  - `enqueue/dequeue`: O(1) amortized under a mutex
  - Dequeued storage is reclaimed immediately
  - `is_lock_free<mpmc_queue<...>>` is `false`

## Roadmap / TODO

- Add an opt-in lock-free MPMC queue only after a tested reclamation strategy exists
- Add epoll/kqueue handle wrappers with `unique_handle`
- Extend backoff strategies and tune per architecture
- Add perf graphs to `docs/` (SPSC throughput, MPMC scalability curves)
