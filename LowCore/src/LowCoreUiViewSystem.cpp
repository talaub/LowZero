#include "LowCoreUiViewSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreUiView.h"

#include "LowUtilProfiler.h"

namespace Low {
  namespace Core {
    namespace UI {
      namespace System {
        namespace View {
          void tick(float p_Delta, Util::EngineState p_State)
          {
            LOW_PROFILE_CPU("Core", "UIViewSystem::TICK");

            UI::View *l_Views = UI::View::living_instances();

            for (uint32_t i = 0u; i < UI::View::living_count(); ++i) {
              UI::View i_View = l_Views[i];

              i_View.set_transform_dirty(false);
            }
          }
        } // namespace View
      }   // namespace System
    }     // namespace UI
  }       // namespace Core
} // namespace Low
