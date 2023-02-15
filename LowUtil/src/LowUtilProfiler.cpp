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

      struct TrackedMemoryAllocation
      {
        String text;
        String module;
        String file;
        String function;
      };

      List<TrackedMemoryAllocation> g_TrackedMemoryAllocations;

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

        Log::profile(l_Profile.module.c_str(), l_LogMsg.c_str());
      }

      void track_memory_allocation(String p_Label, String p_Module,
                                   String p_File, String p_Function)
      {
        TrackedMemoryAllocation l_Allocation;

        l_Allocation.file = p_File;
        l_Allocation.text = p_Label;
        l_Allocation.module = p_Module;
        l_Allocation.function = p_Function;

        g_TrackedMemoryAllocations.push_back(l_Allocation);
      }

      void free_memory_allocation(String p_Label)
      {
        for (auto it = g_TrackedMemoryAllocations.begin();
             it != g_TrackedMemoryAllocations.end(); ++it) {
          if (it->text == p_Label) {
            g_TrackedMemoryAllocations.erase(it);
            return;
          }
        }

        LOW_ASSERT(
            false,
            "Tried to untrack a tracked memory allocation that does not exist");
      }

      void evaluate_memory_allocation()
      {
        for (auto it = g_TrackedMemoryAllocations.begin();
             it != g_TrackedMemoryAllocations.end(); ++it) {
          String i_Text = String("Tracked memory allocation '") + it->text +
                          "' did not get free'd. Function: " + it->function;

          Log::profile(it->module.c_str(), i_Text.c_str());
        }

        LOW_ASSERT(g_TrackedMemoryAllocations.empty(),
                   "Not all tracked memory allocations were free'd");
      }
    } // namespace Profiler
  }   // namespace Util
} // namespace Low
