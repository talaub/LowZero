#pragma once

#include "LowUtilApi.h"

#include "LowUtilContainers.h"

#define LOW_PROFILE_ALLOC(msg)

#define LOW_PROFILE_FREE(msg)

#define LOW_PROFILE_CPU(_grp, _name)

namespace Low {
  namespace Util {
    namespace Profiler {
      LOW_EXPORT void track_memory_allocation(String p_Label,
                                              String p_Module,
                                              String p_File,
                                              String p_Function);
      LOW_EXPORT void free_memory_allocation(String p_Label);

      LOW_EXPORT void evaluate_memory_allocation();

      LOW_EXPORT void flip();
    } // namespace Profiler
  } // namespace Util
} // namespace Low
