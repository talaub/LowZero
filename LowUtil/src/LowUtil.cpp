#include "LowUtil.h"

#include "LowUtilLogger.h"
#include "LowUtilConfig.h"
#include "LowUtilHandle.h"
#include "LowUtilProfiler.h"
#include "LowUtilMemory.h"
#include "LowUtilJobManager.h"
#include "LowUtilFileSystem.h"
#include "LowUtilAssetManager.h"
#include "SDL_video.h"

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
      g_Project.managedPath = "./data/.managed";
      g_Project.visualScriptOut = "./data/.vs_out";

      g_Project.engineRootPath = "./";
      g_Project.engineDataPath = "./LowData";

      Log::initialize();
      Memory::initialize();
      Name::initialize();
      Config::initialize();
      JobManager::initialize();
      AssetManager::initialize();

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

    void tick_handle_reference_resolvers(const float p_Delta);

    void tick(float p_Delta)
    {
      FileSystem::tick(p_Delta);
      AssetManager::tick(p_Delta);
      tick_handle_reference_resolvers(p_Delta);

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
          if (e.window.event == SDL_WINDOWEVENT_CLOSE &&
              e.window.windowID ==
                  SDL_GetWindowID(g_MainWindow.sdlwindow)) {
            g_MainWindow.shouldClose = true;
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

      AssetManager::cleanup();
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

    namespace {
      static String trim_path_separators(const String &p_Path)
      {
        if (p_Path.empty()) {
          return p_Path;
        }

        u32 l_Start = 0;
        u32 l_End = (u32)p_Path.size();

        while (l_Start < l_End &&
               (p_Path[l_Start] == '/' || p_Path[l_Start] == '\\')) {
          ++l_Start;
        }

        while (l_End > l_Start && (p_Path[l_End - 1] == '/' ||
                                   p_Path[l_End - 1] == '\\')) {
          --l_End;
        }

        return p_Path.substr(l_Start, l_End - l_Start);
      }
    } // namespace

    ProjectPathBuilder::ProjectPathBuilder(String p_RootPath)
    {
      String l_Root = p_RootPath;
      while (!l_Root.empty() &&
             (l_Root.back() == '/' || l_Root.back() == '\\')) {
        l_Root.pop_back();
      }

      m_Builder.append(l_Root);
      m_HasPath = !l_Root.empty();
    }

    ProjectPathBuilder &
    ProjectPathBuilder::join(const String &p_PathPart)
    {
      String l_Part = trim_path_separators(p_PathPart);
      if (l_Part.empty()) {
        return *this;
      }

      if (m_HasPath) {
        m_Builder.append("/");
      }

      m_Builder.append(l_Part);
      m_HasPath = true;
      return *this;
    }

    ProjectPathBuilder &
    ProjectPathBuilder::join(const char *p_PathPart)
    {
      return join(String(p_PathPart));
    }

    String ProjectPathBuilder::get() const
    {
      return m_Builder.get();
    }

    ProjectPathBuilder::operator String() const
    {
      return get();
    }

    ProjectPathBuilder project_data_path()
    {
      return ProjectPathBuilder(get_project().dataPath);
    }

    ProjectPathBuilder project_managed_path()
    {
      return ProjectPathBuilder(get_project().managedPath);
    }

    ProjectPathBuilder project_visual_script_out_path()
    {
      return ProjectPathBuilder(get_project().visualScriptOut);
    }

    ProjectPathBuilder project_root_path()
    {
      return ProjectPathBuilder(get_project().rootPath);
    }

    ProjectPathBuilder project_asset_cache_path()
    {
      return ProjectPathBuilder(get_project().assetCachePath);
    }

    ProjectPathBuilder project_editor_images_path()
    {
      return ProjectPathBuilder(get_project().editorImagesPath);
    }

    ProjectPathBuilder engine_root_path()
    {
      return ProjectPathBuilder(get_project().engineRootPath);
    }

    ProjectPathBuilder engine_data_path()
    {
      return ProjectPathBuilder(get_project().engineDataPath);
    }

    String project_data_path(const String &p_RelativePath)
    {
      return project_data_path().join(p_RelativePath).get();
    }

    String project_root_path(const String &p_RelativePath)
    {
      return project_root_path().join(p_RelativePath).get();
    }

    String project_managed_path(const String &p_RelativePath)
    {
      return project_managed_path().join(p_RelativePath).get();
    }

    String project_asset_cache_path(const String &p_RelativePath)
    {
      return project_asset_cache_path().join(p_RelativePath).get();
    }

    String project_editor_images_path(const String &p_RelativePath)
    {
      return project_editor_images_path().join(p_RelativePath).get();
    }

    String
    project_visual_script_out_path(const String &p_RelativePath)
    {
      return project_visual_script_out_path()
          .join(p_RelativePath)
          .get();
    }

    String engine_root_path(const String &p_RelativePath)
    {
      return engine_root_path().join(p_RelativePath).get();
    }

    String engine_data_path(const String &p_RelativePath)
    {
      return engine_data_path().join(p_RelativePath).get();
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
