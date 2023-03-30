#pragma once

#include "LowRendererApi.h"

#include "LowMath.h"

namespace Low {
  namespace Renderer {
    struct RenderFlow;

    LOW_RENDERER_API void initialize();
    LOW_RENDERER_API void tick(float p_Delta);
    LOW_RENDERER_API void late_tick(float p_Delta);
    LOW_RENDERER_API bool window_is_open();
    LOW_RENDERER_API void cleanup();
    LOW_RENDERER_API void
    adjust_renderflow_dimensions(RenderFlow p_RenderFlow,
                                 Math::UVector2 &p_Dimensions);
  } // namespace Renderer
} // namespace Low
