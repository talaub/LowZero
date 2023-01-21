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

    struct LOW_EXPORT Window
    {
      union
      {
        GLFWwindow *m_Glfw;
      };

      void tick() const;
      bool is_open() const;
      void cleanup();
    };

    void LOW_EXPORT window_initialize(Window &p_Window, WindowInit &p_Init);
  } // namespace Renderer
} // namespace Low
