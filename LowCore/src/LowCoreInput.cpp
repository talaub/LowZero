#include "LowCoreInput.h"

#include "LowMath.h"
#include "LowRenderer.h"

#include "LowUtilGlobals.h"
#include "LowUtilLogger.h"

namespace Low {
  namespace Core {
    namespace Input {
      Util::Map<Util::MouseButton, bool> g_MouseSavedState;

      bool keyboard_button_down(Util::KeyboardButton p_Button)
      {
        // TODO: Fix
        return false;
        // return
        // Renderer::get_window().keyboard_button_down(p_Button);
      }

      bool keyboard_button_up(Util::KeyboardButton p_Button)
      {
        // TODO: Fix
        return false;
        // return Renderer::get_window().keyboard_button_up(p_Button);
      }

      bool mouse_button_down(Util::MouseButton p_Button)
      {
        // TODO: Fix
        return false;
        // return Renderer::get_window().mouse_button_down(p_Button);
      }

      bool mouse_button_up(Util::MouseButton p_Button)
      {
        // TODO: Fix
        return false;
        // return Renderer::get_window().mouse_button_up(p_Button);
      }

      bool mouse_button_released(Util::MouseButton p_Button)
      {
        // TODO: Fix
        return false;
        // return g_MouseSavedState[p_Button] &&
        //        mouse_button_up(p_Button);
      }

      bool mouse_button_pressed(Util::MouseButton p_Button)
      {
        // TODO: Fix
        return false;
        // return !g_MouseSavedState[p_Button] &&
        //        mouse_button_down(p_Button);
      }

      void mouse_position(Math::Vector2 &p_Position)
      {
        // TODO: Find correct condition to go by
#if 1
        // Math::Vector2 l_MousePosition;
        // Renderer::get_window().mouse_position(l_MousePosition);
        // Math::Vector2 l_WindowPosition;
        // Renderer::get_window().position(l_WindowPosition);
        // Math::Vector2 l_EditingWidgetPosition =
        //     Util::Globals::get(N(LOW_SCREEN_OFFSET));
        //
        // p_Position = (l_MousePosition + l_WindowPosition) -
        //              l_EditingWidgetPosition;
        p_Position = Math::Vector2(0);
#else
        Renderer::get_window().mouse_position(p_Position);
#endif
      }

      void late_tick(float p_Delta)
      {
        g_MouseSavedState[Util::MouseButton::LEFT] =
            mouse_button_down(Util::MouseButton::LEFT);
        g_MouseSavedState[Util::MouseButton::RIGHT] =
            mouse_button_down(Util::MouseButton::RIGHT);
      }
    } // namespace Input
  } // namespace Core
} // namespace Low
