# syslib — Header-only C++20 Systems Library

A small header-only library focused on practical systems building blocks:

- RAII wrappers for POSIX resources (file descriptors, `FILE*`, mmap regions)
- Utilities: `unique_handle`, `scope_guard`, simple sync wrappers
- Lock-free data structures: bounded SPSC ring buffer, MPMC queue (Michael–Scott)
- TMP-driven policies: queue kind, memory orders, padding, backoff strategies
- Tooling for tests, benchmarks, sanitizers, and static analysis

## Build

- CMake 3.20+
- C++20 compiler (GCC 11+, Clang 13+, AppleClang 14+)

```
cmake -S syslib -B build -DSYSLIB_BUILD_TESTS=ON -DSYSLIB_BUILD_BENCHMARKS=ON
cmake --build build -j
ctest --test-dir build -V
```

## Headers & APIs

- `syslib/scope_guard.hpp`: `scope_guard`, `make_scope_guard`
- `syslib/unique_handle.hpp`: `unique_handle<Handle, Invalid, Deleter>`
- `syslib/file.hpp`: `unique_fd`, `open_fd`, `unique_file`, `fopen_file`
- `syslib/mmap.hpp`: `mapped_region`
- `syslib/sync.hpp`: aliases for mutex/locks/condvar
- `syslib/tmp.hpp`: `queue_policy`, backoff strategies, `is_lock_free`
- `syslib/spsc_ring.hpp`: `spsc_ring<T, Policy>`
- `syslib/mpmc_queue.hpp`: `mpmc_queue<T, Policy>` (MS queue; reclaim stub)
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
- MPMC queue (Michael–Scott)
  - Correct MS enqueue/dequeue algorithm
  - Boilerplate defers node reclamation to destructor (no runtime deletes)
  - Hook point to upgrade to hazard pointers or epoch reclamation

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

Targets to publish:
- SPSC throughput ≥ 1M ops/sec on an M-class Mac (1P/1C)
- MPMC scalability 1→8 producers/consumers, include latency percentiles

## Big-O and Memory Model Notes

- SPSC ring
  - `try_push/try_pop`: O(1) amortized; uses `release`/`acquire` for head/tail
  - Avoids false sharing via padding; use policy to adjust
- MPMC queue
  - `enqueue/dequeue`: O(1) amortized; multiple CAS on pointers
  - Memory reclamation: this boilerplate defers reclamation until destruction.
    For production, use hazard pointers or an epoch-based reclaimer.

## Roadmap / TODO

- Provide hazard-pointer and epoch-based reclaimers conforming to a `Reclaimer` concept
- Add epoll/kqueue handle wrappers with `unique_handle`
- Extend backoff strategies and tune per architecture
- Add perf graphs to `docs/` (SPSC throughput, MPMC scalability curves)

