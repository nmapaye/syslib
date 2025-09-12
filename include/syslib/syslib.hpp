// Header-only umbrella include
#pragma once

#include <cstddef>

#include "tmp.hpp"
#include "scope_guard.hpp"
#include "unique_handle.hpp"
#include "file.hpp"
#include "mmap.hpp"
#include "sync.hpp"
#include "spsc_ring.hpp"
#include "mpmc_queue.hpp"
#include "channel.hpp"

namespace syslib {
inline constexpr std::size_t cache_line_size = 64;
}
