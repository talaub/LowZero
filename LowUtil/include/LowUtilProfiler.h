#pragma once

#include "LowUtilApi.h"

#include "LowUtilContainers.h"

#include <stdint.h>

#define LOW_PROFILE_ALLOC(msg)

#define LOW_PROFILE_FREE(msg)

#define LOW_PROFILE_TOKEN_PASTE_INNER(_x, _y) _x##_y
#define LOW_PROFILE_TOKEN_PASTE(_x, _y)                             \
  LOW_PROFILE_TOKEN_PASTE_INNER(_x, _y)

#define LOW_PROFILE_CPU(_grp, _name)                                \
  Low::Util::Profiler::Scope LOW_PROFILE_TOKEN_PASTE(               \
      l_LowProfileScope, __LINE__)(_grp, _name)

namespace Low {
  namespace Util {
    namespace Profiler {
      struct LOW_EXPORT ScopeSample
      {
        String group;
        String name;
        uint64_t threadId;
        uint32_t depth;
        float startMs;
        float durationMs;
      };

      struct LOW_EXPORT Frame
      {
        uint64_t index;
        float durationMs;
        List<ScopeSample> samples;
      };

      struct LOW_EXPORT Scope
      {
        Scope(const char *p_Group, const char *p_Name);
        ~Scope();

        const char *group;
        const char *name;
        uint64_t start;
        uint32_t depth;
      };

      LOW_EXPORT void track_memory_allocation(String p_Label,
                                              String p_Module,
                                              String p_File,
                                              String p_Function);
      LOW_EXPORT void free_memory_allocation(String p_Label);

      LOW_EXPORT void evaluate_memory_allocation();

      LOW_EXPORT void flip();

      LOW_EXPORT void set_enabled(bool p_Enabled);
      LOW_EXPORT bool is_enabled();
      LOW_EXPORT void clear();
      LOW_EXPORT List<Frame> get_frames();
      LOW_EXPORT Frame get_latest_frame();
    } // namespace Profiler
  } // namespace Util
} // namespace Low
