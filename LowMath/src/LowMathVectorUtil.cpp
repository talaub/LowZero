#include <glm/gtx/euler_angles.hpp>

#include "LowMathVectorUtil.h"

#include <math.h>
#include <string>

namespace Low {
  namespace Math {
    namespace VectorUtil {
      const float PI = 3.14159265359;

      float magnitude_squared(const Vector3 &p_Vector)
      {
        return (p_Vector.x * p_Vector.x) + (p_Vector.y * p_Vector.y) +
               (p_Vector.z * p_Vector.z);
      }

      float magnitude(const Vector3 &p_Vector)
      {
        return sqrt(magnitude_squared(p_Vector));
      }

      float distance_squared(const Vector3 &p_Start,
                             const Vector3 &p_End)
      {
        return magnitude_squared(p_End - p_Start);
      }

      float distance(const Vector3 &p_Start, const Vector3 &p_End)
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
        return glm::quatLookAt(direction, p_Up);
        /*
              float dot = glm::dot(p_Up, direction);
              if (fabs(dot - (-1.0f)) < 0.000001f) {
                return glm::angleAxis(glm::degrees(PI),
           LOW_VECTOR3_UP); } else if (fabs(dot - (1.0f)) < 0.000001f)
           { return glm::quat();
              }

              float angle = -glm::degrees(acosf(dot));

              glm::vec3 cross =
                  glm::normalize(glm::cross(LOW_VECTOR3_FRONT,
           direction)); return glm::normalize(glm::angleAxis(angle,
           cross));
        */
      }

      Vector3 to_euler(Quaternion &p_Rotation)
      {
        const Quaternion &q = p_Rotation;
        float x, y, z, w;
        x = q.x;
        y = q.y;
        z = q.z;
        w = q.w;

        float pitchf, yawf, rollf;
        float test = x * y + z * w;
        if (test > 0.49999f) { // singularity at north pole
          yawf = 2 * atan2(x, w);
          rollf = 1.5707963267948966192313216916398f;
          pitchf = 0;
        } else if (test < -0.49999f) { // singularity at south pole
          yawf = -2 * atan2(x, w);
          rollf = -1.5707963267948966192313216916398f;
          pitchf = 0;
        } else {
          float sqX = x * x;
          float sqY = y * y;
          float sqZ = z * z;
          yawf = atan2(2 * y * w - 2 * x * z, 1 - 2 * sqY - 2 * sqZ);
          rollf = asin(2 * test);
          pitchf =
              atan2(2 * x * w - 2 * y * z, 1 - 2 * sqX - 2 * sqZ);
        }

        glm::vec3 l_Degrees =
            glm::degrees(Math::Vector3(pitchf, yawf, rollf));

        return l_Degrees;
      }

      Quaternion from_euler(Math::Vector3 &p_EulerAngles)
      {
        glm::vec3 l_Radians = glm::radians(p_EulerAngles);

        /*
              glm::mat4 rotationMatrix =
                  glm::rotate(glm::mat4(1.0f), l_Radians.y,
           glm::vec3(0, 1, 0))
           * glm::rotate(glm::mat4(1.0f), l_Radians.x, glm::vec3(1, 0,
           0)) * glm::rotate(glm::mat4(1.0f), l_Radians.z,
           glm::vec3(0, 0, 1));

              glm::quat l_Quat = glm::quat_cast(rotationMatrix);
        */
        Quaternion q;
        // xzy
        // currently the default case (confirmed to be working) :)
        double c1 = cos(l_Radians.y * 0.5);
        double s1 = sin(l_Radians.y * 0.5);
        double c2 = cos(l_Radians.z * 0.5);
        double s2 = sin(l_Radians.z * 0.5);
        double c3 = cos(l_Radians.x * 0.5);
        double s3 = sin(l_Radians.x * 0.5);
        double c1c2 = c1 * c2;
        double s1s2 = s1 * s2;
        q.w = float(c1c2 * c3 - s1s2 * s3);
        q.x = float(c1c2 * s3 + s1s2 * c3);
        q.y = float(s1 * c2 * c3 + c1 * s2 * s3);
        q.z = float(c1 * s2 * c3 - s1 * c2 * s3);

        return q;
      }

      Vector3 rotate_by_quaternion(Vector3 &p_Vec, Quaternion &p_Quat)
      {
        /*
              return p_Vec +
                     2.0f * glm::cross({p_Quat.x, p_Quat.y, p_Quat.z},
                                       cross({p_Quat.x, p_Quat.y,
           p_Quat.z}, p_Vec) + p_Quat.w * p_Vec);
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

      float pitch_degrees(Quaternion &p_Rotation,
                          bool p_ReprojectAxis)
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

      Matrix4x4 to_row_major(const Matrix4x4 p_Matrix)
      {
        glm::mat4 rowMajorMatrix;

        for (int i = 0; i < 4; ++i) {
          for (int j = 0; j < 4; ++j) {
            rowMajorMatrix[j][i] = p_Matrix[i][j];
          }
        }

        return rowMajorMatrix;
      }

      Quaternion between(Vector3 p_From, Vector3 p_To)
      {
        return glm::rotation(p_From, p_To);
      }

      Vector2 lerp(Vector2 &p_Start, Vector2 &p_End, float p_Delta)
      {
        return p_Start + (p_End - p_Start) * p_Delta;
      }

      Vector3 lerp(Vector3 &p_Start, Vector3 &p_End, float p_Delta)
      {
        return p_Start + (p_End - p_Start) * p_Delta;
      }
    } // namespace VectorUtil
  }   // namespace Math
} // namespace Low
