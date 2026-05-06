#include "LowUtilProfiler.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <string>

namespace Low {
  namespace Util {
    namespace Profiler {
      constexpr uint32_t PROFILE_FRAME_COUNT = 256;

      struct TrackedMemoryAllocation
      {
        String text;
        String module;
        String file;
        String function;
      };

      List<TrackedMemoryAllocation> g_TrackedMemoryAllocations;
      List<ScopeSample> g_CurrentFrameSamples;
      List<Frame> g_Frames;
      std::mutex g_Mutex;
      std::atomic<bool> g_Enabled = true;
      uint64_t g_FrameIndex = 0;
      std::atomic<uint64_t> g_FrameStart = 0;
      thread_local uint32_t g_ScopeDepth = 0;

      static uint64_t now_ns()
      {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::steady_clock::now()
                       .time_since_epoch())
            .count();
      }

      static float ns_to_ms(uint64_t p_Ns)
      {
        return static_cast<float>(static_cast<double>(p_Ns) /
                                  1000000.0);
      }

      static uint64_t get_thread_id()
      {
        return static_cast<uint64_t>(
            std::hash<std::thread::id>{}(std::this_thread::get_id()));
      }

      Scope::Scope(const char *p_Group, const char *p_Name)
          : group(p_Group), name(p_Name), start(0), depth(0)
      {
        if (!g_Enabled.load()) {
          return;
        }

        depth = g_ScopeDepth++;
        start = now_ns();

        uint64_t l_ExpectedFrameStart = 0;
        g_FrameStart.compare_exchange_strong(l_ExpectedFrameStart,
                                             start);
      }

      Scope::~Scope()
      {
        if (start == 0) {
          return;
        }

        const uint64_t l_End = now_ns();
        g_ScopeDepth--;

        if (!g_Enabled.load()) {
          return;
        }

        ScopeSample l_Sample;
        l_Sample.group = group;
        l_Sample.name = name;
        l_Sample.threadId = get_thread_id();
        l_Sample.depth = depth;
        l_Sample.startMs = ns_to_ms(start - g_FrameStart.load());
        l_Sample.durationMs = ns_to_ms(l_End - start);

        std::lock_guard<std::mutex> l_Lock(g_Mutex);
        g_CurrentFrameSamples.push_back(l_Sample);
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

        LOW_ASSERT(false, "Tried to untrack a tracked memory "
                          "allocation that does not exist");
      }

      void evaluate_memory_allocation()
      {
        for (auto it = g_TrackedMemoryAllocations.begin();
             it != g_TrackedMemoryAllocations.end(); ++it) {
          String i_Text =
              String("Tracked memory allocation '") + it->text +
              "' did not get free'd. Function: " + it->function;

          Log::begin_log(Log::LogLevel::PROFILE, it->module.c_str())
              << i_Text << LOW_LOG_END;
        }

        LOW_ASSERT(g_TrackedMemoryAllocations.empty(),
                   "Not all tracked memory allocations were free'd");
      }

      void flip()
      {
        if (!g_Enabled.load()) {
          return;
        }

        const uint64_t l_Now = now_ns();

        std::lock_guard<std::mutex> l_Lock(g_Mutex);

        if (g_FrameStart.load() == 0) {
          g_FrameStart = l_Now;
          return;
        }

        Frame l_Frame;
        l_Frame.index = g_FrameIndex++;
        l_Frame.durationMs = ns_to_ms(l_Now - g_FrameStart.load());
        l_Frame.samples.swap(g_CurrentFrameSamples);

        g_Frames.push_back(l_Frame);
        if (g_Frames.size() > PROFILE_FRAME_COUNT) {
          g_Frames.erase(g_Frames.begin());
        }

        g_FrameStart = l_Now;
      }

      void set_enabled(bool p_Enabled)
      {
        std::lock_guard<std::mutex> l_Lock(g_Mutex);
        g_Enabled = p_Enabled;

        if (!g_Enabled.load()) {
          g_CurrentFrameSamples.clear();
          g_FrameStart = 0;
        } else if (g_FrameStart.load() == 0) {
          g_FrameStart = now_ns();
        }
      }

      bool is_enabled()
      {
        return g_Enabled.load();
      }

      void clear()
      {
        std::lock_guard<std::mutex> l_Lock(g_Mutex);
        g_CurrentFrameSamples.clear();
        g_Frames.clear();
        g_FrameIndex = 0;
        g_FrameStart = now_ns();
      }

      List<Frame> get_frames()
      {
        std::lock_guard<std::mutex> l_Lock(g_Mutex);
        return g_Frames;
      }

      Frame get_latest_frame()
      {
        std::lock_guard<std::mutex> l_Lock(g_Mutex);
        if (g_Frames.empty()) {
          Frame l_Frame;
          l_Frame.index = 0;
          l_Frame.durationMs = 0.0f;
          return l_Frame;
        }

        return g_Frames.back();
      }
    } // namespace Profiler
  } // namespace Util
} // namespace Low
