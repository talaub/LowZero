#include "LowMath"

#include <math.h>
#include <cmath>

namespace Low {
  namespace Math {
    namespace Util {
      float lerp(float p_Start, float p_End, float p_Delta)
      {
        return p_Start + (p_End - p_Start) * p_Delta;
      }

      float power2(float p_Power)
      {
        return pow(2.f, p_Power);
      }

      float power(float p_Base, float p_Power)
      {
        return pow(p_Base, p_Power);
      }

      float abs(float p_Num)
      {
        return std::abs(p_Num);
      }
    } // namespace Util
  }   // namespace Math
} // namespace Low
