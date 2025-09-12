// Simple sync helpers (wrappers / typedefs)
#pragma once

#include <condition_variable>
#include <mutex>

namespace syslib {

using mutex = std::mutex;
using unique_lock = std::unique_lock<std::mutex>;
using lock_guard = std::lock_guard<std::mutex>;
using condition_variable = std::condition_variable;

}  // namespace syslib

