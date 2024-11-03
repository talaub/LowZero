#pragma once

#define SDL_MAIN_HANDLED
#include <SDL.h>

namespace Low {
  namespace Util {
    struct Window
    {
      SDL_Window *sdlwindow;

      static Window &get_main_window();
    };

  } // namespace Util
} // namespace Low
