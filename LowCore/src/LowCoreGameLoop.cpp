#include "LowCoreGameLoop.h"

#include "LowCore.h"
#include "LowCoreRegionSystem.h"
#include "LowCoreTransformSystem.h"
#include "LowCoreLightSystem.h"
#include "LowCoreMeshRendererSystem.h"
#include "LowCoreTaskScheduler.h"
#include "LowCoreTransform.h"
#include "LowCorePhysicsSystem.h"
#include "LowCoreNavmeshSystem.h"

#include "LowCoreMeshResource.h"

#include <chrono>
#include <microprofile.h>

#include "LowUtilLogger.h"
#include "LowUtilContainers.h"
#include "LowUtilJobManager.h"

#include "LowUtilProfiler.h"

#include "LowRenderer.h"

namespace Low {
  namespace Core {
    namespace GameLoop {
      using namespace std::chrono;

      bool g_Running = false;
      uint32_t g_LastFps = 0u;

      uint64_t g_Frames = 0ull;

      Util::List<System::TickCallback> g_TickCallbacks;

      static void execute_ticks(float p_Delta)
      {
        static bool l_FirstRun = true;
        Renderer::tick(p_Delta, get_engine_state());
        System::Transform::tick(p_Delta, get_engine_state());
        System::Light::tick(p_Delta, get_engine_state());
        System::Region::tick(p_Delta, get_engine_state());
        if (!l_FirstRun) {
          System::Physics::tick(p_Delta, get_engine_state());
        }
        System::Navmesh::tick(p_Delta, get_engine_state());

        MeshResource::update();

        for (auto it = g_TickCallbacks.begin(); it != g_TickCallbacks.end();
             ++it) {
          (*it)(p_Delta, get_engine_state());
        }

        // test_mono();

        System::MeshRenderer::tick(p_Delta, get_engine_state());

        l_FirstRun = false;
      }

      static void execute_late_ticks(float p_Delta)
      {
        static bool l_FirstRun = true;
        if (!l_FirstRun) {
          System::Physics::late_tick(p_Delta, get_engine_state());
        }
        System::Transform::late_tick(p_Delta, get_engine_state());

        Renderer::late_tick(p_Delta, get_engine_state());
        l_FirstRun = false;
      }

      static void run()
      {
        const auto l_TimeStep = 1'000'000'000ns / 144;

        auto l_Accumulator = 0ns;
        auto l_SecondAccumulator = 0ns;
        auto l_LastTime = steady_clock::now();

        int l_Fps = 0;

        while (g_Running) {
          {
            LOW_PROFILE_CPU("Core", "Tick");
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

            g_Frames++;
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
          }

          Util::Profiler::flip();
        }
      }

      void initialize()
      {
        System::Physics::initialize();
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
