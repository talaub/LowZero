#pragma once

#include "LowRendererApi.h"

namespace Low {
  namespace Renderer {
    LOW_EXPORT void initialize();
    LOW_EXPORT void tick(float p_Delta);
    LOW_EXPORT bool window_is_open();
    LOW_EXPORT void cleanup();
  } // namespace Renderer
} // namespace Low
