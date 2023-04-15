#pragma once

#include "LowMathApi.h"
#include "LowMath.h"

namespace Low {
  namespace Math {
    namespace QuaternionUtil {
      LOW_EXPORT Quaternion from_angle_axis(const float p_RadianAngle,
                                            const Vector3 &p_Axis);

      LOW_EXPORT void set_roll(Quaternion &p_Quaternion,
                               const float p_RollRadians);
    } // namespace QuaternionUtil
  }   // namespace Math
} // namespace Low
