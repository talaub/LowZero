#pragma once

#include "LowMathApi.h"
#include "LowMath.h"

namespace Low {
  namespace Math {
    namespace VectorUtil {
      LOW_EXPORT float magnitude_squared(const Vector3 &p_Vector);
      LOW_EXPORT float magnitude(const Vector3 &p_Vector);

      LOW_EXPORT float distance_squared(const Vector3 &p_Start,
                                        const Vector3 &p_End);
      LOW_EXPORT float distance(const Vector3 &p_Start,
                                const Vector3 &p_End);

      LOW_EXPORT Math::Vector3
      rotate_by_quaternion(Math::Vector3 &p_Vec,
                           Math::Quaternion &p_Quat);

      LOW_EXPORT Vector3 normalize(Vector3 &p_Vector);

      LOW_EXPORT Vector3 direction(Quaternion &p_Rotation);
      LOW_EXPORT Quaternion from_direction(Vector3 &p_Direction,
                                           Vector3 &p_Up);

      LOW_EXPORT float pitch(Quaternion &p_Rotation,
                             bool p_ReprojectAxis = true);
      LOW_EXPORT float yaw(Quaternion &p_Rotation,
                           bool p_ReprojectAxis = true);
      LOW_EXPORT float roll(Quaternion &p_Rotation,
                            bool p_ReprojectAxis = true);

      LOW_EXPORT float pitch_degrees(Quaternion &p_Rotation,
                                     bool p_ReprojectAxis = true);
      LOW_EXPORT float yaw_degrees(Quaternion &p_Rotation,
                                   bool p_ReprojectAxis = true);
      LOW_EXPORT float roll_degrees(Quaternion &p_Rotation,
                                    bool p_ReprojectAxis = true);

      LOW_EXPORT Vector3 to_euler(Quaternion &p_Rotation);

      LOW_EXPORT Quaternion from_euler(Math::Vector3 &p_EulerAngles);

      LOW_EXPORT Matrix4x4 to_row_major(const Matrix4x4 p_Matrix);

      LOW_EXPORT Quaternion between(Vector3 p_From, Vector3 p_To);

      LOW_EXPORT Vector2 lerp(Vector2 &p_Start, Vector2 &p_End,
                              float p_Delta);
      LOW_EXPORT Vector3 lerp(Vector3 &p_Start, Vector3 &p_End,
                              float p_Delta);
    } // namespace VectorUtil
  }   // namespace Math
} // namespace Low
