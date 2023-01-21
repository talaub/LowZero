#include "LowRendererWindow.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

namespace Low {
  namespace Renderer {
    static void glfw_window_initialize(Window &p_Window, WindowInit &p_Init)
    {
      LOW_ASSERT(glfwInit(), "Failed to initialize GLFW");

      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

      p_Window.m_Glfw =
          glfwCreateWindow(p_Init.dimensions.x, p_Init.dimensions.y,
                           p_Init.title, nullptr, nullptr);
    }

    void window_initialize(Window &p_Window, WindowInit &p_Init)
    {
#ifdef LOW_RENDERER_WINDOW_API_GLFW
      glfw_window_initialize(p_Window, p_Init);
#else
      LOW_ASSERT(false, "No window API set");
#endif
    }

    static void window_tick_glfw()
    {
      glfwPollEvents();
    }

    void Window::tick() const
    {
#ifdef LOW_RENDERER_WINDOW_API_GLFW
      window_tick_glfw();
#else
      LOW_ASSERT(false, "No window API set");
#endif
    }

    static bool window_isopen_glfw(const Window &p_Window)
    {
      return !glfwWindowShouldClose(p_Window.m_Glfw);
    }

    bool Window::is_open() const
    {
#ifdef LOW_RENDERER_WINDOW_API_GLFW
      return window_isopen_glfw(*this);
#else
      return LOW_ASSERT(false, "No window API set");
#endif
    }

    static void window_cleanup_glfw(const Window &p_Window)
    {
      glfwDestroyWindow(p_Window.m_Glfw);
      glfwTerminate();
    }

    void Window::cleanup()
    {
#ifdef LOW_RENDERER_WINDOW_API_GLFW
      window_cleanup_glfw(*this);
#else
      LOW_ASSERT(false, "No window API set");
#endif
    }
  } // namespace Renderer
} // namespace Low
