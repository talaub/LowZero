#pragma once

#include "LowRendererApi.h"

namespace Low {
  namespace Renderer {
    LOW_RENDERER_API void initialize();
    LOW_RENDERER_API void tick(float p_Delta);
    LOW_RENDERER_API void late_tick(float p_Delta);
    LOW_RENDERER_API bool window_is_open();
    LOW_RENDERER_API void cleanup();
  } // namespace Renderer
} // namespace Low
