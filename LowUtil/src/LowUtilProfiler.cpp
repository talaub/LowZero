#include "LowUtilProfiler.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include <chrono>
#include <string>

namespace Low {
  namespace Util {
    namespace Profiler {
      struct CpuProfile
      {
        String text;
        String module;
        String file;
        String function;
        std::chrono::steady_clock::time_point beginTime;
      };

      Stack<CpuProfile> g_CpuProfiles;

      void start_cpu_profile(String p_Text, String p_Module, String p_File,
                             String p_Function)
      {
        CpuProfile l_Profile;

        l_Profile.file = p_File;
        l_Profile.text = p_Text;
        l_Profile.module = p_Module;
        l_Profile.function = p_Function;
        l_Profile.beginTime = std::chrono::steady_clock::now();

        g_CpuProfiles.push(l_Profile);
      }

      void end_cpu_profile()
      {
        std::chrono::steady_clock::time_point l_EndTime =
            std::chrono::steady_clock::now();

        LOW_ASSERT(!g_CpuProfiles.empty(),
                   "Tried to pop from cpu profile stack. But stack was empty");

        CpuProfile l_Profile = g_CpuProfiles.top();
        g_CpuProfiles.pop();

        uint64_t l_TimeDifferenceNanos =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                l_EndTime - l_Profile.beginTime)
                .count();
        uint64_t l_TimeDifferenceMillis =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                l_EndTime - l_Profile.beginTime)
                .count();

        String l_LogMsg =
            l_Profile.text + " (" + l_Profile.function + ") took ";

        if (l_TimeDifferenceMillis > 0) {
          l_LogMsg += String(std::to_string(l_TimeDifferenceMillis).c_str()) +
                      " milliseconds.";
        } else if (l_TimeDifferenceNanos > 50000) {
          double l_MilliDouble = l_TimeDifferenceNanos / 1000000.0;
          l_LogMsg +=
              String(std::to_string(l_MilliDouble).c_str()) + " milliseconds.";
        } else {
          l_LogMsg += String(std::to_string(l_TimeDifferenceNanos).c_str()) +
                      " nanoseconds.";
        }

        LOW_LOG_PROFILE(l_LogMsg.c_str());
      }
    } // namespace Profiler
  }   // namespace Util
} // namespace Low
