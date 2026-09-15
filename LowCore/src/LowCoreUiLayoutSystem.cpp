#include "LowCoreUiLayoutSystem.h"

#include "LowCoreUiLayout.h"
#include "LowUtilProfiler.h"

namespace Low {
  namespace Core {
    namespace UI {
      namespace System {
        namespace Layout {
          void tick(float p_Delta, Util::EngineState p_State)
          {
            LOW_PROFILE_CPU("Core", "UiLayoutSystem::TICK");

            Component::Layout *l_Layouts =
                Component::Layout::living_instances();

            for (uint32_t i = 0u;
                 i < Component::Layout::living_count(); ++i) {
              Component::Layout i_Layout = l_Layouts[i];
              i_Layout.set_world_updated(false);
            }

            for (uint32_t i = 0u;
                 i < Component::Layout::living_count(); ++i) {
              Component::Layout i_Layout = l_Layouts[i];
              if (i_Layout.is_world_dirty()) {
                i_Layout.resolve();
              }
            }
          }
        } // namespace Layout
      }   // namespace System
    }     // namespace UI
  }       // namespace Core
} // namespace Low
