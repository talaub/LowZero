#include "LowUtil.h"

#include "LowUtilLogger.h"
#include "LowUtilConfig.h"
#include "LowUtilHandle.h"
#include "LowUtilProfiler.h"
#include "LowUtilMemory.h"
#include "LowUtilJobManager.h"
#include "LowUtilFileSystem.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

namespace Low {
  namespace Util {
    Project g_Project;

    Low::Util::Window g_MainWindow;

    void initialize()
    {
      g_Project.dataPath = "./data";
      g_Project.rootPath = "./";
      g_Project.assetCachePath = "./data/.asset_cache";
      g_Project.editorImagesPath = "./data/.editor_images";

      g_Project.engineRootPath = "./";
      g_Project.engineDataPath = "./LowData";

      Log::initialize();
      Memory::initialize();
      Name::initialize();
      Config::initialize();
      JobManager::initialize();

      {
        SDL_Init(SDL_INIT_VIDEO);

        SDL_WindowFlags window_flags =
            (SDL_WindowFlags)(SDL_WINDOW_VULKAN |
                              SDL_WINDOW_RESIZABLE);

        g_MainWindow.shouldClose = false;
        g_MainWindow.minimized = false;
        g_MainWindow.sdlwindow = SDL_CreateWindow(
            "LowEngine", SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED, 1280, 720, window_flags);
      }

      LOW_LOG_INFO << "Util initialized" << LOW_LOG_END;
    }

    void tick(float p_Delta)
    {
      FileSystem::tick(p_Delta);

      SDL_Event e;

      while (SDL_PollEvent(&e) != 0) {
        // close the window when user alt-f4s or clicks the X button
        if (e.type == SDL_QUIT)
          g_MainWindow.shouldClose = true;

        if (e.type == SDL_WINDOWEVENT) {
          if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
            g_MainWindow.minimized = true;
          }
          if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
            g_MainWindow.minimized = false;
          }
        }

        for (u32 i = 0;
             i < Window::get_main_window().eventCallbacks.size();
             ++i) {
          Window::get_main_window().eventCallbacks[i](&e);
        }
      }

      // do not draw if we are minimized
      if (g_MainWindow.minimized) {
        // throttle the speed to avoid the endless spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }

    void cleanup()
    {
      SDL_DestroyWindow(g_MainWindow.sdlwindow);

      JobManager::cleanup();
      Name::cleanup();
      Memory::cleanup();

      Profiler::evaluate_memory_allocation();

      LOW_LOG_INFO << "Util shutdown" << LOW_LOG_END;
      Log::cleanup();
    }

    const Project &get_project()
    {
      return g_Project;
    }

    Window &Window::get_main_window()
    {
      return g_MainWindow;
    }

    void Window::get_size(int *p_Width, int *p_Height)
    {
      SDL_GetWindowSize(sdlwindow, p_Width, p_Height);
    }
  } // namespace Util
} // namespace Low
