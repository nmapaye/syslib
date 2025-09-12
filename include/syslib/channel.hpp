// Queue selection via policy
#pragma once

#include "tmp.hpp"
#include "spsc_ring.hpp"
#include "mpmc_queue.hpp"

namespace syslib {

template <class T, class Policy = queue_policy<>>
using channel = std::conditional_t<Policy::kind == queue_kind::spsc,
                                   spsc_ring<T, Policy>,
                                   mpmc_queue<T, Policy>>;

}  // namespace syslib

