#include "LowCoreInput.h"

#include "LowMath.h"
#include "LowRenderer.h"

#include "LowUtilGlobals.h"
#include "LowUtilLogger.h"
#include "SDL_events.h"

namespace Low {
  namespace Core {
    namespace Input {
      Util::Map<Util::MouseButton, bool> g_MouseSavedState;
      // HACK: Temp
      bool g_Clicked = false;
      Math::Vector2 g_MousePosition;

      bool keyboard_button_down(Util::KeyboardButton p_Button)
      {

        // FIX: Fix
        return false;

        // Renderer::get_window().keyboard_button_down(p_Button);
      }

      bool keyboard_button_up(Util::KeyboardButton p_Button)
      {
        // FIX: Fix
        return false;
        // return Renderer::get_window().keyboard_button_up(p_Button);
      }

      bool mouse_button_down(Util::MouseButton p_Button)
      {
        // FIX: It's hard coded to left
        return g_Clicked;
        // return Renderer::get_window().mouse_button_down(p_Button);
      }

      bool mouse_button_up(Util::MouseButton p_Button)
      {
        // FIX: Fix
        return false;
        // return Renderer::get_window().mouse_button_up(p_Button);
      }

      bool mouse_button_released(Util::MouseButton p_Button)
      {
        // FIX: Fix
        return false;
        // return g_MouseSavedState[p_Button] &&
        //        mouse_button_up(p_Button);
      }

      bool mouse_button_pressed(Util::MouseButton p_Button)
      {

        // FIX: Fix
        return false;
        // return !g_MouseSavedState[p_Button] &&
        //        mouse_button_down(p_Button);
      }

      void mouse_position(Math::Vector2 &p_Position)
      {
        // HACK: Find correct condition to go by

        p_Position = g_MousePosition;

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

        int l_MouseX, l_MouseY;
        SDL_GetMouseState(&l_MouseX, &l_MouseY);
        g_MousePosition.x = l_MouseX;
        g_MousePosition.y = l_MouseY;
      }

      static bool process_events(const SDL_Event *p_Event)
      {
        if (p_Event->type == SDL_MOUSEBUTTONDOWN) {
          g_Clicked = true;
        }
        if (p_Event->type == SDL_MOUSEBUTTONUP) {
          g_Clicked = false;
        }
        return true;
      }

      void initialize()
      {
        Util::Window::get_main_window().eventCallbacks.push_back(
            &process_events);
      }

      void cleanup()
      {
      }
    } // namespace Input
  } // namespace Core
} // namespace Low
