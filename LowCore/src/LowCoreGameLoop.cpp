#include "LowCoreGameLoop.h"

#include "LowCoreRegionSystem.h"
#include "LowCoreTransformSystem.h"
#include "LowCoreLightSystem.h"
#include "LowCoreMeshRendererSystem.h"
#include "LowCoreTaskScheduler.h"
#include "LowCoreTransform.h"

#include <chrono>
#include <microprofile.h>

#include "LowUtilLogger.h"
#include "LowUtilContainers.h"

#include "LowRenderer.h"

namespace Low {
  namespace Core {
    namespace GameLoop {
      using namespace std::chrono;

      bool g_Running = false;
      uint32_t g_LastFps = 0u;

      Util::List<System::TickCallback> g_TickCallbacks;

      static void execute_ticks(float p_Delta)
      {
        Renderer::tick(p_Delta);
        System::Region::tick(p_Delta);
        System::Transform::tick(p_Delta);
        System::Light::tick(p_Delta);

        for (auto it = g_TickCallbacks.begin(); it != g_TickCallbacks.end();
             ++it) {
          (*it)(p_Delta);
        }

        System::MeshRenderer::tick(p_Delta);
      }

      static void execute_late_ticks(float p_Delta)
      {
        System::Transform::late_tick(p_Delta);

        Renderer::late_tick(p_Delta);
      }

      static void run()
      {
        const auto l_TimeStep = 1'000'000'000ns / 144;

        auto l_Accumulator = 0ns;
        auto l_SecondAccumulator = 0ns;
        auto l_LastTime = steady_clock::now();

        int l_Fps = 0;

        while (g_Running) {
          auto i_Now = steady_clock::now();
          l_Accumulator += i_Now - l_LastTime;
          l_LastTime = i_Now;

          if (l_Accumulator < l_TimeStep) {
            TaskScheduler::tick();
            continue;
          }

          l_SecondAccumulator += l_Accumulator;

          l_Fps++;

          float l_DeltaTime =
              duration_cast<duration<float>>(l_Accumulator).count();

          execute_ticks(l_DeltaTime);

          if (l_SecondAccumulator > 1'000'000'000ns) {
            l_SecondAccumulator = 0ns;
            g_LastFps = l_Fps;
            l_Fps = 0;
          }

          l_Accumulator = 0ns;

          if (!Renderer::window_is_open()) {
            stop();
            continue;
          }

          execute_late_ticks(l_DeltaTime);

          MicroProfileFlip(nullptr);
        }
      }

      void initialize()
      {
      }

      void start()
      {
        g_Running = true;
        run();
      }

      void stop()
      {
        g_Running = false;
      }

      void cleanup()
      {
      }

      void register_tick_callback(System::TickCallback p_Callback)
      {
        g_TickCallbacks.push_back(p_Callback);
      }

      uint32_t get_fps()
      {
        return g_LastFps;
      }
    } // namespace GameLoop
  }   // namespace Core
} // namespace Low
