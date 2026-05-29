#pragma once

#include "LowMathApi.h"
#include "LowMath.h"

namespace Low {
  namespace Math {
    namespace QuaternionUtil {
      LOW_EXPORT Quaternion get_identity();

      LOW_EXPORT Quaternion from_angle_axis(const float p_RadianAngle,
                                            const Vector3 &p_Axis);

      LOW_EXPORT void set_roll(Quaternion &p_Quaternion,
                               const float p_RollRadians);

      [[nodiscard]] LOW_EXPORT float dot(const Quaternion p_A,
                                         const Quaternion p_B);
      [[nodiscard]] LOW_EXPORT float
      magnitude(const Quaternion p_Quaternion);
      [[nodiscard]] LOW_EXPORT Quaternion
      normalize(const Quaternion p_Quaternion);
      [[nodiscard]] LOW_EXPORT Quaternion
      slerp(const Quaternion p_A, const Quaternion p_B, float p_T);
    } // namespace QuaternionUtil
  }   // namespace Math
} // namespace Low
