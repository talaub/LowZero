#pragma once

#include "LowUtilApi.h"

#include "LowUtilContainers.h"
#include "LowUtilString.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

namespace Low {
  namespace Util {
    LOW_EXPORT void initialize();
    LOW_EXPORT void tick(float p_Delta);
    LOW_EXPORT void cleanup();

    struct Project
    {
      String dataPath;
      String rootPath;
      String assetCachePath;
      String editorImagesPath;
      String engineRootPath;
      String engineDataPath;
      String managedPath;
    };
    LOW_EXPORT const Project &get_project();

    struct LOW_EXPORT ProjectPathBuilder
    {
      ProjectPathBuilder() = default;
      ProjectPathBuilder(String p_RootPath);

      ProjectPathBuilder &join(const String &p_PathPart);
      ProjectPathBuilder &join(const char *p_PathPart);

      String get() const;
      operator String() const;

    private:
      StringBuilder m_Builder;
      bool m_HasPath = false;
    };

    LOW_EXPORT ProjectPathBuilder project_data_path();
    LOW_EXPORT ProjectPathBuilder project_root_path();
    LOW_EXPORT ProjectPathBuilder project_asset_cache_path();
    LOW_EXPORT ProjectPathBuilder project_managed_path();
    LOW_EXPORT ProjectPathBuilder project_editor_images_path();
    LOW_EXPORT ProjectPathBuilder engine_root_path();
    LOW_EXPORT ProjectPathBuilder engine_data_path();

    LOW_EXPORT String project_data_path(const String &p_RelativePath);
    LOW_EXPORT String project_root_path(const String &p_RelativePath);
    LOW_EXPORT String
    project_asset_cache_path(const String &p_RelativePath);
    LOW_EXPORT String
    project_editor_images_path(const String &p_RelativePath);
    LOW_EXPORT String
    project_managed_path(const String &p_RelativePath);
    LOW_EXPORT String engine_root_path(const String &p_RelativePath);
    LOW_EXPORT String engine_data_path(const String &p_RelativePath);

    struct LOW_EXPORT Window
    {
      SDL_Window *sdlwindow;

      bool shouldClose = false;
      bool minimized = false;

      typedef bool (*EventCallback)(const SDL_Event *);
      Util::List<EventCallback> eventCallbacks;

      void get_size(int *p_Width, int *p_Height);

      static Window &get_main_window();
    };
  } // namespace Util
} // namespace Low
