#include "LowCoreNavigationSystem.h"

#include "LowCoreNavigation.h"
#include "LowCoreScene.h"

#include "LowUtil.h"
#include "LowUtilConfig.h"
#include "LowUtilProfiler.h"

namespace Low {
  namespace Core {
    namespace System {
      namespace Navigation {
        static Low::Util::Name setting_name(const char *p_Name)
        {
          Low::Util::String l_FullName = "navigation/runtime/";
          l_FullName += p_Name;
          return LOW_NAME(l_FullName.c_str());
        }

        static float get_runtime_min_y()
        {
          return Low::Util::get_project().settings.get_float(
              setting_name("min_y"), -1000.0f);
        }

        static float get_runtime_max_y()
        {
          return Low::Util::get_project().settings.get_float(
              setting_name("max_y"), 1000.0f);
        }

        static uint32_t get_max_tiles_per_tick()
        {
          return Low::Util::get_project().settings.get_u32(
              setting_name("max_tiles_per_tick"), 1u);
        }

        static uint32_t get_max_dirty_tiles_queued_per_tick()
        {
          return Low::Util::get_project().settings.get_u32(
              setting_name("max_dirty_tiles_queued_per_tick"), 8u);
        }

        static uint32_t get_eviction_interval_ticks()
        {
          return Low::Util::get_project().settings.get_u32(
              setting_name("eviction_interval_ticks"), 30u);
        }

        static uint32_t get_max_evictions_per_tick()
        {
          return Low::Util::get_project().settings.get_u32(
              setting_name("max_evictions_per_tick"), 8u);
        }

        void tick(float p_Delta, Util::EngineState p_State)
        {
          if (p_State != Util::EngineState::PLAYING) {
            // return;
          }

          LOW_PROFILE_CPU("Core", "NavigationSystem::TICK");

          const float l_MinY = get_runtime_min_y();
          const float l_MaxY = get_runtime_max_y();
          const uint32_t l_MaxTilesPerTick = get_max_tiles_per_tick();
          const uint32_t l_MaxDirtyTilesQueuedPerTick =
              get_max_dirty_tiles_queued_per_tick();
          const uint32_t l_EvictionIntervalTicks =
              get_eviction_interval_ticks();
          const uint32_t l_MaxEvictionsPerTick =
              get_max_evictions_per_tick();

          for (uint32_t i = 0u; i < Scene::living_count(); ++i) {
            Scene i_Scene = Scene::living_instances()[i];
            if (!i_Scene.is_alive() || !i_Scene.is_loaded()) {
              continue;
            }

            Low::Core::Navigation::World i_World =
                i_Scene.get_navigation_world();
            if (!i_World.is_alive()) {
              continue;
            }

            Low::Core::Navigation::update_invoker_tiles(
                i_World, l_MinY, l_MaxY);
            if (Low::Core::Navigation::should_run_eviction(
                    i_World, l_EvictionIntervalTicks)) {
              Low::Core::Navigation::evict_tiles_outside_invokers(
                  i_World, l_MaxEvictionsPerTick);
            }
            Low::Core::Navigation::queue_dirty_tiles(
                i_World, l_MaxDirtyTilesQueuedPerTick);
            Low::Core::Navigation::update_tile_builds(
                i_World, l_MaxTilesPerTick);
          }
        }
      } // namespace Navigation
    } // namespace System
  } // namespace Core
} // namespace Low
