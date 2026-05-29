#include "LowMathQuaternionUtil.h"

#include "LowMathVectorUtil.h"

#include <glm/gtc/quaternion.hpp>
#include <math.h>
#include <string>

namespace Low {
  namespace Math {
    namespace QuaternionUtil {
      const float PI = 3.14159265359;

      Quaternion from_angle_axis(const float p_RadianAngle,
                                 const Vector3 &p_Axis)
      {
        float l_HalfAngle = 0.5f * p_RadianAngle;
        float l_Sin = sin(l_HalfAngle);

        Quaternion l_Result;
        l_Result.w = cos(l_HalfAngle);
        l_Result.x = l_Sin * p_Axis.x;
        l_Result.y = l_Sin * p_Axis.y;
        l_Result.z = l_Sin * p_Axis.z;

        return l_Result;
      }

      void set_roll(Quaternion &p_Quaternion,
                    const float p_RollRadians)
      {
        float l_Pitch = VectorUtil::pitch(p_Quaternion);
        float l_Yaw = VectorUtil::yaw(p_Quaternion);

        p_Quaternion = VectorUtil::from_euler(
            glm::degrees(Vector3(l_Pitch, l_Yaw, p_RollRadians)));
      }

      Quaternion get_identity()
      {
        static Quaternion q(1.f, 0.f, 0.f, 0.f);
        return q;
      }

      float dot(const Quaternion p_A, const Quaternion p_B)
      {
        return p_A.x * p_B.x + p_A.y * p_B.y + p_A.z * p_B.z +
               p_A.w * p_B.w;
      }

      float magnitude(const Quaternion p_Quaternion)
      {
        return sqrt(dot(p_Quaternion, p_Quaternion));
      }

      Quaternion normalize(const Quaternion p_Quaternion)
      {
        const float l_Magnitude = magnitude(p_Quaternion);
        if (l_Magnitude <= LOW_MATH_EPSILON) {
          return get_identity();
        }
        return glm::normalize(p_Quaternion);
      }

      Quaternion slerp(const Quaternion p_A, const Quaternion p_B,
                       float p_T)
      {
        return glm::slerp(p_A, p_B, p_T);
      }
    } // namespace QuaternionUtil
  } // namespace Math
} // namespace Low
