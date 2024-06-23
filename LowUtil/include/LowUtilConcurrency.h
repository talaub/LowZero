#pragma once

#include "LowUtilApi.h"

#include <thread>
#include <mutex>

#include "LowUtilContainers.h"

namespace Low {
  namespace Util {
    typedef std::thread Thread;
    typedef std::mutex Mutex;
  } // namespace Util
} // namespace Low
