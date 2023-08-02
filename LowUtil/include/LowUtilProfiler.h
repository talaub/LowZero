#pragma once

#include "LowUtilApi.h"

#include "LowUtilContainers.h"

#include "microprofile.h"

#define LOW_PROFILE_ALLOC(msg)                                                 \
  Low::Util::Profiler::track_memory_allocation(#msg, LOW_MODULE_NAME,          \
                                               __FILE__, __FUNCTION__)

#define LOW_PROFILE_FREE(msg) Low::Util::Profiler::free_memory_allocation(#msg)

#define LOW_PROFILE_CPU(_grp, _name) MICROPROFILE_SCOPEI(_grp, _name, MP_GREEN)

namespace Low {
  namespace Util {
    namespace Profiler {
      LOW_EXPORT void track_memory_allocation(String p_Label, String p_Module,
                                              String p_File, String p_Function);
      LOW_EXPORT void free_memory_allocation(String p_Label);

      LOW_EXPORT void evaluate_memory_allocation();

      LOW_EXPORT void flip();
    } // namespace Profiler
  }   // namespace Util
} // namespace Low
