#include "LowCoreTransformSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreUiDisplay.h"
#include "LowCoreInput.h"

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

            Math::Vector2 l_MousePosition;
            Input::mouse_position(l_MousePosition);

            Component::Display l_HoveredDisplay = 0;

            for (uint32_t i = 0u;
                 i < Component::Display::living_count(); ++i) {
              Component::Display i_Display = l_Displays[i];

              i_Display.set_world_updated(false);

              if (!i_Display.get_element()
                       .get_view()
                       .is_view_template() &&
                  !i_Display.get_element().is_click_passthrough()) {
                if (i_Display.point_is_in_bounding_box(
                        l_MousePosition)) {
                  if (!l_HoveredDisplay.is_alive()) {
                    l_HoveredDisplay = i_Display;
                  } else if (l_HoveredDisplay.get_absolute_layer() <
                             i_Display.get_absolute_layer()) {
                    l_HoveredDisplay = i_Display;
                  }
                }
              }
            }

            if (l_HoveredDisplay.is_alive()) {
              LOW_LOG_DEBUG
                  << "Hovering: "
                  << l_HoveredDisplay.get_element().get_name()
                  << LOW_LOG_END;
            }
          }

        } // namespace Display
      }   // namespace System
    }     // namespace UI
  }       // namespace Core
} // namespace Low
