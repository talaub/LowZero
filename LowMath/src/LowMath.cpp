#include "LowMath.h"

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

      float clamp(float p_Num, float p_Low, float p_High)
      {
        if (p_Num > p_High) {
          return p_High;
        }
        if (p_Num < p_Low) {
          return p_Low;
        }
        return p_Num;
      }

      uint32_t clamp(uint32_t p_Num, uint32_t p_Low, uint32_t p_High)
      {
        if (p_Num > p_High) {
          return p_High;
        }
        if (p_Num < p_Low) {
          return p_Low;
        }
        return p_Num;
      }

      float asin(float p_Value)
      {
        return std::asin(p_Value);
      }

      float atan2(float p_Value0, float p_Value1)
      {
        return std::atan2(p_Value0, p_Value1);
      }
    } // namespace Util
  }   // namespace Math
} // namespace Low
