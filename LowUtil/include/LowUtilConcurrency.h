#pragma once

#include "LowUtilApi.h"

#include <thread>
#include <mutex>

#include "LowUtilContainers.h"

namespace Low {
  namespace Util {
    typedef std::thread Thread;

    using SharedMutex = std::shared_mutex;
    using Mutex = std::mutex;

    // Exclusive lock (write lock)
    template <typename MutexT = SharedMutex>
    using UniqueLock = std::unique_lock<MutexT>;

    // Shared lock (read lock)
    template <typename MutexT = SharedMutex>
    using SharedLock = std::shared_lock<MutexT>;
  } // namespace Util
} // namespace Low
