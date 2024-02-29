#include "LowCoreTransformSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreUiDisplay.h"

#include "LowUtilProfiler.h"

namespace Low {
  namespace Core {
    namespace UI {
      namespace System {
        namespace Display {
          void tick(float p_Delta, Util::EngineState p_State)
          {
            LOW_PROFILE_CPU("Core", "UiDisplaySystem::TICK");

            Component::Display *l_Displays =
                Component::Display::living_instances();

            for (uint32_t i = 0u;
                 i < Component::Display::living_count(); ++i) {
              Component::Display i_Display = l_Displays[i];

              i_Display.set_world_updated(false);
            }
          }

        } // namespace Display
      }   // namespace System
    }     // namespace UI
  }       // namespace Core
} // namespace Low
