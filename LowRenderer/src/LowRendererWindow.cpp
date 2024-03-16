#include "LowRendererWindow.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "../../LowDependencies/stb/stb_image.h"

namespace Low {
  namespace Renderer {
    static void glfw_window_initialize(Window &p_Window,
                                       WindowInit &p_Init)
    {
      LOW_ASSERT(glfwInit(), "Failed to initialize GLFW");

      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

      Util::String l_Path = LOW_DATA_PATH;
      l_Path += "/_internal/assets/editor_icons/raw/logo.png";

      GLFWimage images[1];
      images[0].pixels =
          stbi_load(l_Path.c_str(), &images[0].width,
                    &images[0].height, 0, 4); // rgba channels

      p_Window.m_Glfw =
          glfwCreateWindow(p_Init.dimensions.x, p_Init.dimensions.y,
                           p_Init.title, nullptr, nullptr);

      glfwSetWindowIcon(p_Window.m_Glfw, 1, images);
      stbi_image_free(images[0].pixels);
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

    namespace Input {
      static uint8_t keyboard_button(Util::KeyboardButton p_Button)
      {
        switch (p_Button) {
        case Util::KeyboardButton::Q:
          return GLFW_KEY_Q;
        case Util::KeyboardButton::W:
          return GLFW_KEY_W;
        case Util::KeyboardButton::E:
          return GLFW_KEY_E;
        case Util::KeyboardButton::A:
          return GLFW_KEY_A;
        case Util::KeyboardButton::S:
          return GLFW_KEY_S;
        case Util::KeyboardButton::D:
          return GLFW_KEY_D;
        default:
          LOW_ASSERT(false, "Unknown keyboard button");
        }
      }

      static uint8_t mouse_button(Util::MouseButton p_Button)
      {
        switch (p_Button) {
        case Util::MouseButton::LEFT:
          return GLFW_MOUSE_BUTTON_LEFT;
        case Util::MouseButton::RIGHT:
          return GLFW_MOUSE_BUTTON_RIGHT;
        default:
          LOW_ASSERT(false, "Unknown mouse button");
        }
      }
    } // namespace Input

    bool Window::keyboard_button_down(Util::KeyboardButton p_Button)
    {
      return glfwGetKey(m_Glfw, Input::keyboard_button(p_Button)) ==
             GLFW_PRESS;
    }

    bool Window::keyboard_button_up(Util::KeyboardButton p_Button)
    {
      return glfwGetKey(m_Glfw, Input::keyboard_button(p_Button)) ==
             GLFW_RELEASE;
    }

    bool Window::mouse_button_down(Util::MouseButton p_Button)
    {
      return glfwGetMouseButton(
                 m_Glfw, Input::mouse_button(p_Button)) == GLFW_PRESS;
    }

    bool Window::mouse_button_up(Util::MouseButton p_Button)
    {
      return glfwGetMouseButton(m_Glfw,
                                Input::mouse_button(p_Button)) ==
             GLFW_RELEASE;
    }

    void Window::mouse_position(Math::Vector2 &p_Position)
    {
      double l_PosX, l_PosY;

      glfwGetCursorPos(m_Glfw, &l_PosX, &l_PosY);

      p_Position.x = l_PosX;
      p_Position.y = l_PosY;
    }

    void Window::position(Math::Vector2 &p_Position)
    {
      int x, y;
      glfwGetWindowPos(m_Glfw, &x, &y);
      p_Position.x = x;
      p_Position.y = y;
    }
  } // namespace Renderer
} // namespace Low
