#pragma once

#include "LowUtilApi.h"

#include "LowUtilContainers.h"

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
    };

    LOW_EXPORT const Project &get_project();

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
