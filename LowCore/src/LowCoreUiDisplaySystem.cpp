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

            Component::Display *l_Displays =
                Component::Display::living_instances();

            Math::Vector2 l_MousePosition;
            Input::mouse_position(l_MousePosition);

            Component::Display l_HoveredDisplay = 0;

            for (uint32_t i = 0u;
                 i < Component::Display::living_count(); ++i) {
              Component::Display i_Display = l_Displays[i];

              i_Display.set_world_updated(false);

              View i_View = i_Display.get_element().get_view();

              if ((!i_View.is_alive() ||
                   !i_View.is_view_template()) &&
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
