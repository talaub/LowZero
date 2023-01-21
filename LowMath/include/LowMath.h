#pragma once

#include "LowMathApi.h"

#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

#include <stdint.h>

namespace Low {
  namespace Math {
    typedef glm::vec2 Vector2;
    typedef glm::vec3 Vector3;
    typedef glm::vec4 Vector4;

    typedef glm::vec3 ColorRGB;
    typedef glm::vec4 Color;

    typedef glm::uvec2 UVector2;

    typedef glm::quat Quaternion;

    typedef glm::mat4 Matrix4x4;

    namespace Util {
      LOW_EXPORT float lerp(float p_Start, float p_End, float p_Delta);
      LOW_EXPORT float power2(float p_Power);
      LOW_EXPORT float power(float p_Base, float p_Power);
      LOW_EXPORT float abs(float p_Num);
    } // namespace Util
  }   // namespace Math
} // namespace Low
