#include <glm/gtx/euler_angles.hpp>

#include "LowMathVectorUtil.h"

#include <math.h>
#include <string>

namespace Low {
  namespace Math {
    namespace VectorUtil {
      const float PI = 3.14159265359;

      float magnitude_squared(Vector3 &p_Vector)
      {
        return (p_Vector.x * p_Vector.x) + (p_Vector.y * p_Vector.y) +
               (p_Vector.z * p_Vector.z);
      }

      float magnitude(Vector3 &p_Vector)
      {
        return sqrt(magnitude_squared(p_Vector));
      }

      float distance_squared(Vector3 &p_Start, Vector3 &p_End)
      {
        return magnitude_squared(p_End - p_Start);
      }

      float distance(Vector3 &p_Start, Vector3 &p_End)
      {
        return sqrt(distance_squared(p_Start, p_End));
      }

      Vector3 normalize(Vector3 &p_Vector)
      {
        return glm::normalize(p_Vector);
      }

      Vector3 direction(Quaternion &p_Rotation)
      {
        return rotate_by_quaternion(LOW_VECTOR3_FRONT, p_Rotation);
      }

      Quaternion from_direction(Vector3 &p_Direction, Vector3 &p_Up)
      {
        glm::vec3 direction = glm::normalize(p_Direction);
        float dot = glm::dot(p_Up, direction);
        if (fabs(dot - (-1.0f)) < 0.000001f) {
          return glm::angleAxis(glm::degrees(PI), LOW_VECTOR3_UP);
        } else if (fabs(dot - (1.0f)) < 0.000001f) {
          return glm::quat();
        }

        float angle = -glm::degrees(acosf(dot));

        glm::vec3 cross =
            glm::normalize(glm::cross(LOW_VECTOR3_FRONT, direction));
        return glm::normalize(glm::angleAxis(angle, cross));
      }

      Vector3 to_euler(Quaternion &p_Rotation)
      {
        glm::vec3 l_Radians = glm::eulerAngles(p_Rotation);

        glm::vec3 l_Degrees = glm::degrees(l_Radians);

        return l_Degrees;
      }

      Quaternion from_euler(Math::Vector3 &p_EulerAngles)
      {
        glm::vec3 l_Radians = glm::radians(p_EulerAngles);

        glm::mat4 rotationMatrix =
            glm::rotate(glm::mat4(1.0f), l_Radians.y, glm::vec3(0, 1, 0)) *
            glm::rotate(glm::mat4(1.0f), l_Radians.x, glm::vec3(1, 0, 0)) *
            glm::rotate(glm::mat4(1.0f), l_Radians.z, glm::vec3(0, 0, 1));

        glm::quat l_Quat = glm::quat_cast(rotationMatrix);
        return l_Quat;
      }

      Vector3 rotate_by_quaternion(Vector3 &p_Vec, Quaternion &p_Quat)
      {
        /*
              return p_Vec +
                     2.0f * glm::cross({p_Quat.x, p_Quat.y, p_Quat.z},
                                       cross({p_Quat.x, p_Quat.y, p_Quat.z},
           p_Vec) + p_Quat.w * p_Vec);
        */
        return p_Quat * p_Vec;
      }

      float pitch(Quaternion &p_Rotation, bool p_ReprojectAxis)
      {
        float l_Result = 0.0f;

        if (p_ReprojectAxis) {
          float fTx = 2.0f * p_Rotation.x;
          float fTz = 2.0f * p_Rotation.z;
          float fTwx = fTx * p_Rotation.w;
          float fTxx = fTx * p_Rotation.x;
          float fTyz = fTz * p_Rotation.y;
          float fTzz = fTz * p_Rotation.z;

          l_Result = Util::atan2(fTyz + fTwx, 1.0f - (fTxx + fTzz));
        } else {
          l_Result = glm::pitch(p_Rotation);
        }

        return l_Result;
      }

      float yaw(Quaternion &p_Rotation, bool p_ReprojectAxis)
      {
        float l_Result = 0.0f;

        if (p_ReprojectAxis) {
          float fTx = 2.0f * p_Rotation.x;
          float fTy = 2.0f * p_Rotation.y;
          float fTz = 2.0f * p_Rotation.z;
          float fTwy = fTy * p_Rotation.w;
          float fTxx = fTx * p_Rotation.x;
          float fTxz = fTz * p_Rotation.x;
          float fTyy = fTy * p_Rotation.y;

          l_Result = Util::atan2(fTxz + fTwy, 1.0f - (fTxx + fTyy));
        } else {
          l_Result = glm::yaw(p_Rotation);
        }

        return l_Result;
      }

      float roll(Quaternion &p_Rotation, bool p_ReprojectAxis)
      {
        float l_Result = 0.0f;

        if (p_ReprojectAxis) {
          float fTy = 2.0f * p_Rotation.y;
          float fTz = 2.0f * p_Rotation.z;
          float fTwz = fTz * p_Rotation.w;
          float fTxy = fTy * p_Rotation.x;
          float fTyy = fTy * p_Rotation.y;
          float fTzz = fTz * p_Rotation.z;

          l_Result = Util::atan2(fTxy + fTwz, 1.0f - (fTyy + fTzz));
        } else {
          l_Result = glm::roll(p_Rotation);
        }

        return l_Result;
      }

      float pitch_degrees(Quaternion &p_Rotation, bool p_ReprojectAxis)
      {
        return glm::degrees(pitch(p_Rotation, p_ReprojectAxis));
      }
      float yaw_degrees(Quaternion &p_Rotation, bool p_ReprojectAxis)
      {
        return glm::degrees(yaw(p_Rotation, p_ReprojectAxis));
      }
      float roll_degrees(Quaternion &p_Rotation, bool p_ReprojectAxis)
      {
        return glm::degrees(roll(p_Rotation, p_ReprojectAxis));
      }
    } // namespace VectorUtil
  }   // namespace Math
} // namespace Low
