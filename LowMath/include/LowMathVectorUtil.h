#pragma once

#include "LowMathApi.h"
#include "LowMath.h"

namespace Low {
  namespace Math {
    namespace VectorUtil {
      LOW_EXPORT float magnitude_squared(Vector3 &p_Vector);
      LOW_EXPORT float magnitude(Vector3 &p_Vector);

      LOW_EXPORT float distance_squared(Vector3 &p_Start, Vector3 &p_End);
      LOW_EXPORT float distance(Vector3 &p_Start, Vector3 &p_End);

      LOW_EXPORT Vector3 normalize(Vector3 &p_Vector);

      LOW_EXPORT const char *to_string(Vector2 &p_Vector);
      LOW_EXPORT const char *to_string(Vector3 &p_Vector);
      LOW_EXPORT const char *to_string(Vector4 &p_Vector);
    } // namespace VectorUtil
  }   // namespace Math
} // namespace Low
