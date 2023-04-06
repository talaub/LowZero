#pragma once

#include "LowMath.h"

namespace Low {
  namespace Renderer {
    struct DirectionalLight
    {
      Math::Vector3 direction;
      Math::ColorRGB color;
    };
  } // namespace Renderer
} // namespace Low
