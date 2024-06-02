#include "LowUtilLogger.h"
#include "LowUtil.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include "LowRendererCompatibility.h"

#include "LowRenderer.h"

#include "imgui_impl_sdl2.h"

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment,
                     size_t alignmentOffset, const char *pName,
                     int flags, unsigned debugFlags, const char *file,
                     int line)
{
  return malloc(size);
}

Low::Util::Window g_MainWindow;

namespace Low {
  namespace Util {
    Window &Window::get_main_window()
    {
      return g_MainWindow;
    }
  } // namespace Util
} // namespace Low
  //
Low::Math::UVector2 g_Dimensions{1700, 900};

void draw()
{
  Low::Renderer::tick(0.0f);
}

bool stop_rendering = false;

void run()
{
  SDL_Event e;
  bool bQuit = false;

  // main loop
  while (!bQuit) {
    // Handle events on queue
    while (SDL_PollEvent(&e) != 0) {
      // close the window when user alt-f4s or clicks the X button
      if (e.type == SDL_QUIT)
        bQuit = true;

      if (e.type == SDL_WINDOWEVENT) {
        if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
          stop_rendering = true;
        }
        if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
          stop_rendering = false;
        }
      }

      ImGui_ImplSDL2_ProcessEvent(&e);
    }

    // do not draw if we are minimized
    if (stop_rendering) {
      // throttle the speed to avoid the endless spinning
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    Low::Renderer::check_window_resize(0.0f);

    draw();
  }
}

void init()
{
  Low::Renderer::initialize();
}

void cleanup()
{
  Low::Renderer::cleanup();
}

int main()
{
  Low::Util::initialize();

  SDL_Init(SDL_INIT_VIDEO);

  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

  g_MainWindow.sdlwindow =
      SDL_CreateWindow("LowRenderer2", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, g_Dimensions.x,
                       g_Dimensions.y, window_flags);

  init();

  run();

  cleanup();

  LOW_LOG_DEBUG << "Testrenderer2" << LOW_LOG_END;

  SDL_DestroyWindow(g_MainWindow.sdlwindow);

  Low::Util::cleanup();
  return 0;
}
