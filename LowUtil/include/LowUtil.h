#pragma once

#include "LowUtilApi.h"

namespace Low {
  namespace Util {
    LOW_EXPORT void initialize();
    LOW_EXPORT void tick(float p_Delta);
    LOW_EXPORT void cleanup();
  } // namespace Util
} // namespace Low
