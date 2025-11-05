#pragma once

#include "LowMathApi.h"
#include "LowMath.h"

namespace Low {
  namespace Math {
    namespace VectorUtil {
      [[nodiscard]] LOW_EXPORT float
      magnitude_squared(const Vector3 p_Vector);
      [[nodiscard]] LOW_EXPORT float
      magnitude(const Vector3 p_Vector);

      [[nodiscard]] LOW_EXPORT float
      distance_squared(const Vector3 p_Start, const Vector3 p_End);
      [[nodiscard]] LOW_EXPORT float distance(const Vector3 p_Start,
                                              const Vector3 p_End);

      [[nodiscard]] LOW_EXPORT Math::Vector3
      rotate_by_quaternion(const Math::Vector3 p_Vec,
                           const Math::Quaternion p_Quat);

      [[nodiscard]] LOW_EXPORT Vector3
      normalize(const Vector3 p_Vector);

      [[nodiscard]] LOW_EXPORT Vector3
      direction(const Quaternion p_Rotation);
      [[nodiscard]] LOW_EXPORT Quaternion
      from_direction(const Vector3 p_Direction, const Vector3 p_Up);

      [[nodiscard]] LOW_EXPORT float
      pitch(const Quaternion p_Rotation,
            const bool p_ReprojectAxis = true);
      [[nodiscard]] LOW_EXPORT float
      yaw(const Quaternion p_Rotation,
          const bool p_ReprojectAxis = true);
      [[nodiscard]] LOW_EXPORT float
      roll(const Quaternion p_Rotation,
           const bool p_ReprojectAxis = true);

      [[nodiscard]] LOW_EXPORT float
      pitch_degrees(const Quaternion p_Rotation,
                    const bool p_ReprojectAxis = true);
      [[nodiscard]] LOW_EXPORT float
      yaw_degrees(const Quaternion p_Rotation,
                  const bool p_ReprojectAxis = true);
      [[nodiscard]] LOW_EXPORT float
      roll_degrees(const Quaternion p_Rotation,
                   const bool p_ReprojectAxis = true);

      [[nodiscard]] LOW_EXPORT Vector3
      to_euler(const Quaternion p_Rotation);

      [[nodiscard]] LOW_EXPORT Quaternion
      from_euler(const Math::Vector3 p_EulerAngles);

      [[nodiscard]] LOW_EXPORT Matrix4x4
      to_row_major(const Matrix4x4 p_Matrix);

      [[nodiscard]] LOW_EXPORT Quaternion
      between(const Vector3 p_From, const Vector3 p_To);

      [[nodiscard]] LOW_EXPORT Vector2 lerp(const Vector2 p_Start,
                                            const Vector2 p_End,
                                            const float p_Delta);
      [[nodiscard]] LOW_EXPORT Vector3 lerp(const Vector3 p_Start,
                                            const Vector3 p_End,
                                            const float p_Delta);
    } // namespace VectorUtil
  } // namespace Math
} // namespace Low
