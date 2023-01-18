#include "LowMathVectorUtil.h"

namespace Low {
  namespace Math {
    float magnitude_squared(Vector3 &p_Vector)
    {
      return ((double)p_Vector.x * p_Vector.x) +
             ((double)p_Vector.y * p_Vector.y) +
             ((double)p_Vector.z * p_Vector.z);
    }
  } // namespace Math
} // namespace Low
