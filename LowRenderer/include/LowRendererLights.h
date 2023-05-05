#pragma once

#include "LowMath.h"

namespace Low {
  namespace Renderer {
    struct DirectionalLight
    {
      Math::Quaternion rotation;
      Math::ColorRGB color;
    };
  } // namespace Renderer
} // namespace Low
