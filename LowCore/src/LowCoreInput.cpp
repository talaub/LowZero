#include "LowCoreInput.h"

#include "LowMath.h"
#include "LowRenderer.h"

#include "LowUtilEnums.h"
#include "LowUtilGlobals.h"
#include "LowUtilLogger.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"

namespace Low {
  namespace Core {
    namespace Input {
      Util::Map<Util::MouseButton, bool> g_MouseSavedState;
      Util::Map<Util::KeyboardButton, bool> g_KeyboardSavedState;

      // HACK: Remove hardcode test stuff
      bool g_Clicked = false;

      Math::Vector2 g_MousePosition;

      static SDL_Scancode
      keyboard_button_to_scancode(Util::KeyboardButton p_Button)
      {
        switch (p_Button) {
        case Util::KeyboardButton::Q:
          return SDL_SCANCODE_Q;
        case Util::KeyboardButton::W:
          return SDL_SCANCODE_W;
        case Util::KeyboardButton::E:
          return SDL_SCANCODE_E;
        case Util::KeyboardButton::R:
          return SDL_SCANCODE_R;
        case Util::KeyboardButton::T:
          return SDL_SCANCODE_T;
        case Util::KeyboardButton::Y:
          return SDL_SCANCODE_Y;
        case Util::KeyboardButton::U:
          return SDL_SCANCODE_U;
        case Util::KeyboardButton::I:
          return SDL_SCANCODE_I;
        case Util::KeyboardButton::O:
          return SDL_SCANCODE_O;
        case Util::KeyboardButton::P:
          return SDL_SCANCODE_P;
        case Util::KeyboardButton::A:
          return SDL_SCANCODE_A;
        case Util::KeyboardButton::S:
          return SDL_SCANCODE_S;
        case Util::KeyboardButton::D:
          return SDL_SCANCODE_D;
        case Util::KeyboardButton::F:
          return SDL_SCANCODE_F;
        case Util::KeyboardButton::G:
          return SDL_SCANCODE_G;
        case Util::KeyboardButton::H:
          return SDL_SCANCODE_H;
        case Util::KeyboardButton::J:
          return SDL_SCANCODE_J;
        case Util::KeyboardButton::K:
          return SDL_SCANCODE_K;
        case Util::KeyboardButton::L:
          return SDL_SCANCODE_L;
        case Util::KeyboardButton::Z:
          return SDL_SCANCODE_Z;
        case Util::KeyboardButton::X:
          return SDL_SCANCODE_X;
        case Util::KeyboardButton::C:
          return SDL_SCANCODE_C;
        case Util::KeyboardButton::V:
          return SDL_SCANCODE_V;
        case Util::KeyboardButton::B:
          return SDL_SCANCODE_B;
        case Util::KeyboardButton::N:
          return SDL_SCANCODE_N;
        case Util::KeyboardButton::M:
          return SDL_SCANCODE_M;
        }

        return SDL_SCANCODE_UNKNOWN;
      }

      bool keyboard_button_down(Util::KeyboardButton p_Button)
      {
        const SDL_Scancode l_Scancode =
            keyboard_button_to_scancode(p_Button);
        if (l_Scancode == SDL_SCANCODE_UNKNOWN) {
          return false;
        }

        const Uint8 *l_State = SDL_GetKeyboardState(nullptr);
        return l_State && l_State[l_Scancode];
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
