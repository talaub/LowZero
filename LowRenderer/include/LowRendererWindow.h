#pragma once

#include "LowRendererApi.h"

#include "LowMath.h"

#include "LowUtilEnums.h"

#include <GLFW/glfw3.h>

namespace Low {
  namespace Renderer {
    struct WindowInit
    {
      Low::Math::UVector2 dimensions;
      const char *title;
    };

    struct LOW_RENDERER_API Window
    {
      union
      {
        GLFWwindow *m_Glfw;
      };

      void tick() const;
      bool is_open() const;
      void cleanup();

      bool keyboard_button_down(Util::KeyboardButton p_Button);
      bool keyboard_button_up(Util::KeyboardButton p_Button);

      bool mouse_button_down(Util::MouseButton p_Button);
      bool mouse_button_up(Util::MouseButton p_Button);
    };

    void LOW_RENDERER_API window_initialize(Window &p_Window,
                                            WindowInit &p_Init);
  } // namespace Renderer
} // namespace Low
