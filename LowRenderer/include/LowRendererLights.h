#pragma once

#include "LowMath.h"

namespace Low {
  namespace Renderer {
    struct DirectionalLight
    {
      Math::Quaternion rotation;
      Math::ColorRGB color;
    };

    struct PointLight
    {
      Math::Vector3 position;
      Math::ColorRGB color;
    };
  } // namespace Renderer
} // namespace Low
