#include "LowMathQuaternionUtil.h"

#include "LowMathVectorUtil.h"

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

      void set_roll(Quaternion &p_Quaternion, const float p_RollRadians)
      {
        float l_Pitch = VectorUtil::pitch(p_Quaternion);
        float l_Yaw = VectorUtil::yaw(p_Quaternion);

        p_Quaternion = VectorUtil::from_euler(
            glm::degrees(Math::Vector3(l_Pitch, l_Yaw, p_RollRadians)));
      }

      Quaternion get_identity()
      {
        static Quaternion q(1.f, 0.f, 0.f, 0.f);
        return q;
      }
    } // namespace QuaternionUtil
  }   // namespace Math
} // namespace Low
