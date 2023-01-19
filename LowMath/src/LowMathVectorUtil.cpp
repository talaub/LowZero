#include "LowMathVectorUtil.h"

#include <math.h>
#include <string>

namespace Low {
  namespace Math {
    namespace VectorUtil {
      float magnitude_squared(Vector3 &p_Vector)
      {
        return ((double)p_Vector.x * p_Vector.x) +
               ((double)p_Vector.y * p_Vector.y) +
               ((double)p_Vector.z * p_Vector.z);
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

      const char *to_string(Vector2 &p_Vector)
      {
        std::string l_String = "";
        l_String += "(" + std::to_string(p_Vector.x) + ", " +
                    std::to_string(p_Vector.y) + ")";

        return l_String.c_str();
      }

      const char *to_string(Vector3 &p_Vector)
      {
        std::string l_String = "";
        l_String += "(" + std::to_string(p_Vector.x) + ", " +
                    std::to_string(p_Vector.y) + ", " +
                    std::to_string(p_Vector.z) + ")";

        return l_String.c_str();
      }

      const char *to_string(Vector4 &p_Vector)
      {
        std::string l_String = "";
        l_String += "(" + std::to_string(p_Vector.x) + ", " +
                    std::to_string(p_Vector.y) + ", " +
                    std::to_string(p_Vector.z) + ", " +
                    std::to_string(p_Vector.w) + ")";

        return l_String.c_str();
      }
    } // namespace VectorUtil
  }   // namespace Math
} // namespace Low
