#include "LowCoreInput.h"

#include "LowMath.h"
#include "LowMathVectorUtil.h"
#include "LowRenderer.h"

#include "LowUtilEnums.h"
#include "LowUtilGlobals.h"
#include "LowUtilLogger.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_video.h"

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
        int l_MouseX = 0;
        int l_MouseY = 0;
        SDL_GetGlobalMouseState(&l_MouseX, &l_MouseY);

        Math::Vector2 l_MousePosition((float)l_MouseX,
                                      (float)l_MouseY);

        Math::Vector2 l_ViewportPosition =
            Util::Globals::get(N(LOW_SCREEN_OFFSET));

        p_Position = l_MousePosition - l_ViewportPosition;
      }

      bool mouse_world_ray(Math::Vector3 *p_Origin,
                           Math::Vector3 *p_Direction)
      {
        if (!p_Origin || !p_Direction) {
          return false;
        }

        Renderer::RenderView l_RenderView =
            Renderer::get_game_renderview();
        if (!l_RenderView.is_alive()) {
          return false;
        }

        const Math::UVector2 l_Dimensions =
            l_RenderView.get_dimensions();
        if (l_Dimensions.x == 0u || l_Dimensions.y == 0u) {
          return false;
        }

        Math::Vector2 l_MousePosition;
        mouse_position(l_MousePosition);

        if (l_MousePosition.x < 0.0f ||
            l_MousePosition.y < 0.0f ||
            l_MousePosition.x > (float)l_Dimensions.x ||
            l_MousePosition.y > (float)l_Dimensions.y) {
          return false;
        }

        const float l_NdcX =
            ((l_MousePosition.x / (float)l_Dimensions.x) * 2.0f) -
            1.0f;
        const float l_NdcY =
            1.0f -
            ((l_MousePosition.y / (float)l_Dimensions.y) * 2.0f);

        const float l_Aspect =
            (float)l_Dimensions.x / (float)l_Dimensions.y;
        const float l_HalfFovTan =
            glm::tan(glm::radians(l_RenderView.get_camera_fov()) *
                     0.5f);

        Math::Vector3 l_Forward = l_RenderView.get_camera_direction();
        if (glm::dot(l_Forward, l_Forward) < LOW_MATH_EPSILON) {
          l_Forward = LOW_VECTOR3_FRONT;
        } else {
          l_Forward = Math::VectorUtil::normalize(l_Forward);
        }
        Math::Vector3 l_Right =
            glm::cross(l_Forward, LOW_VECTOR3_UP);
        if (glm::dot(l_Right, l_Right) < LOW_MATH_EPSILON) {
          l_Right = LOW_VECTOR3_RIGHT;
        } else {
          l_Right = Math::VectorUtil::normalize(l_Right);
        }

        const Math::Vector3 l_Up =
            Math::VectorUtil::normalize(glm::cross(l_Right, l_Forward));

        *p_Origin = l_RenderView.get_camera_position();
        *p_Direction = Math::VectorUtil::normalize(
            l_Forward + (l_Right * l_NdcX * l_HalfFovTan * l_Aspect) +
            (l_Up * l_NdcY * l_HalfFovTan));

        return true;
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
