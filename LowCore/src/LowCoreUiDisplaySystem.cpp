#include "LowCoreTransformSystem.h"

#include "LowCoreUiWidgetInstance.h"
#include "LowUtilAssert.h"
#include "LowUtilEnums.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreUiDisplay.h"
#include "LowCoreInput.h"
#include "LowCoreUi.h"

namespace Low {
  namespace Core {
    namespace UI {
      namespace System {
        namespace Display {
          void tick(float p_Delta, Util::EngineState p_State)
          {
            LOW_PROFILE_CPU("Core", "UiDisplaySystem::TICK");

            for (Component::Display i_Display :
                 Component::Display::ms_Dirty) {
              if (i_Display.is_alive()) {
                if (i_Display.is_dirty()) {
                  i_Display.recalculate_world_transform();
                }
              }
            }
            Component::Display::ms_Dirty.clear();

            Math::Vector2 l_MousePosition;
            Input::mouse_position(l_MousePosition);

            Component::Display l_HoveredDisplay = 0;

            if (l_HoveredDisplay.is_alive()) {
              set_hovered_element(l_HoveredDisplay.get_element());
            } else {
              set_hovered_element(0);
            }
            if (Input::mouse_button_down(Util::MouseButton::LEFT)) {
              Element l_Element = get_hovered_element();
              if (l_Element.is_alive()) {
                WidgetInstance l_WidgetInstance =
                    l_Element.get_widget_instance();
                if (l_WidgetInstance.is_alive()) {
                  ControllerInstance l_ControllerInstance =
                      l_WidgetInstance.get_controller_instance();
                  if (l_ControllerInstance.is_alive()) {
                    l_ControllerInstance.handle_click(l_Element);
                  }
                }
              }
            }
          }

        } // namespace Display
      } // namespace System
    } // namespace UI
  } // namespace Core
} // namespace Low
