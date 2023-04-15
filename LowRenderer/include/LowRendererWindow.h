#pragma once

#include "LowRendererApi.h"

#include "LowMath.h"

#include <GLFW/glfw3.h>

namespace Low {
  namespace Renderer {
    struct WindowInit
    {
      Low::Math::UVector2 dimensions;
      const char *title;
    };

    namespace Input {
      LOW_RENDERER_API enum class KeyboardButton {
        Q,
        W,
        E,
        R,
        T,
        Y,
        U,
        I,
        O,
        P,
        A,
        S,
        D,
        F,
        G,
        H,
        J,
        K,
        L,
        Z,
        X,
        C,
        V,
        B,
        N,
        M
      };

      LOW_RENDERER_API enum class MouseButton { LEFT, RIGHT };
    } // namespace Input

    struct LOW_RENDERER_API Window
    {
      union
      {
        GLFWwindow *m_Glfw;
      };

      void tick() const;
      bool is_open() const;
      void cleanup();

      bool keyboard_button_down(Input::KeyboardButton p_Button);
      bool keyboard_button_up(Input::KeyboardButton p_Button);

      bool mouse_button_down(Input::MouseButton p_Button);
      bool mouse_button_up(Input::MouseButton p_Button);
    };

    void LOW_RENDERER_API window_initialize(Window &p_Window,
                                            WindowInit &p_Init);
  } // namespace Renderer
} // namespace Low
