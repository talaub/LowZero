#include "LowUtilProfiler.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include <chrono>
#include <string>

namespace Low {
  namespace Util {
    namespace Profiler {
      struct TrackedMemoryAllocation
      {
        String text;
        String module;
        String file;
        String function;
      };

      List<TrackedMemoryAllocation> g_TrackedMemoryAllocations;

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

          Log::begin_log(Log::LogLevel::PROFILE, it->module.c_str())
              << i_Text << LOW_LOG_END;
        }

        LOW_ASSERT(g_TrackedMemoryAllocations.empty(),
                   "Not all tracked memory allocations were free'd");
      }

      void flip()
      {
        MicroProfileFlip(nullptr);
      }
    } // namespace Profiler
  }   // namespace Util
} // namespace Low
