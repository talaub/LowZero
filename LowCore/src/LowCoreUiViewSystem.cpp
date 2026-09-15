#include "LowCoreUiViewSystem.h"

#include "LowCoreUiScreen.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowUtilProfiler.h"

namespace Low {
  namespace Core {
    namespace UI {
      namespace System {
        namespace View {
          void tick(float p_Delta, Util::EngineState p_State)
          {
            // LOW_PROFILE_CPU("Core", "UIViewSystem::TICK");

            // UI::View *l_Views = UI::View::living_instances();

            // for (uint32_t i = 0u; i < UI::View::living_count();
            // ++i) { UI::View i_View = l_Views[i];

            // i_View.set_transform_dirty(false);
            //}

            UI::Screen *l_Screens = UI::Screen::living_instances();
            for (u32 i = 0; i < UI::Screen::living_count(); ++i) {
              UI::Screen i_Screen = l_Screens[i];

              i_Screen.set_dirty(false);
            }
          }
        } // namespace View
      } // namespace System
    } // namespace UI
  } // namespace Core
} // namespace Low
