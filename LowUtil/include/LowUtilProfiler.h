#pragma once

#include "LowUtilApi.h"

#include "LowUtilContainers.h"

#define LOW_PROFILE_START(msg)                                                 \
  Low::Util::Profiler::start_cpu_profile(#msg, LOW_MODULE_NAME, __FILE__,      \
                                         __FUNCTION__)

#define LOW_PROFILE_END() Low::Util::Profiler::end_cpu_profile()

#define LOW_PROFILE_ALLOC(msg)                                                 \
  Low::Util::Profiler::track_memory_allocation(#msg, LOW_MODULE_NAME,          \
                                               __FILE__, __FUNCTION__)

#define LOW_PROFILE_FREE(msg) Low::Util::Profiler::free_memory_allocation(#msg)

namespace Low {
  namespace Util {
    namespace Profiler {
      LOW_EXPORT void start_cpu_profile(String p_Text, String p_Module,
                                        String p_File, String p_Function);
      LOW_EXPORT void end_cpu_profile();

      LOW_EXPORT void track_memory_allocation(String p_Label, String p_Module,
                                              String p_File, String p_Function);
      LOW_EXPORT void free_memory_allocation(String p_Label);

      LOW_EXPORT void evaluate_memory_allocation();
    } // namespace Profiler
  }   // namespace Util
} // namespace Low
