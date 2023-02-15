#include "LowMathVectorUtil.h"

#include <math.h>
#include <string>

namespace Low {
  namespace Math {
    namespace VectorUtil {
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

    } // namespace VectorUtil
  }   // namespace Math
} // namespace Low
