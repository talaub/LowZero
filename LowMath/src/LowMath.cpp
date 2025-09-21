#include "LowMath.h"

#include <cstdlib>
#include <math.h>
#include <cmath>
#include <random>

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

      float floor(float p_Num)
      {
        return std::floor(p_Num);
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

      float random()
      {
        static std::random_device
            rd; // Seed for the random number generator
        static std::mt19937 gen(rd()); // Mersenne Twister generator
        static std::uniform_real_distribution<float> dist(
            0.0f, 1.0f); // Distribution in [0, 1)

        return dist(gen); // Generate a random number in [0.0, 1.0)
      }

      float random_range(float p_Min, float p_Max)
      {
        // Ensure min <= max to avoid logic errors
        if (p_Min > p_Max) {
          std::swap(p_Min, p_Max);
        }

        return p_Min + random() * (p_Max - p_Min);
      }

      float random_range_int(int p_Min, int p_Max)
      {
        // Ensure min <= max to avoid logic errors
        if (p_Min > p_Max) {
          std::swap(p_Min, p_Max);
        }

        return p_Min + static_cast<int>(
                           floor(random() * (p_Max - p_Min + 1)));
      }

      bool random_percent(u8 p_Percent)
      {
        return random_percent(((float)p_Percent) / 100.0f);
      }

      bool random_percent(float p_Percent)
      {
        return random() <= p_Percent;
      }

      u32 next_power_of_two(u32 p_Value)
      {
        if (p_Value == 0) {
          return 1;
        }

        // Decrement first to handle exact powers of two correctly
        p_Value--;
        p_Value |= p_Value >> 1;
        p_Value |= p_Value >> 2;
        p_Value |= p_Value >> 4;
        p_Value |= p_Value >> 8;
        p_Value |= p_Value >> 16;
        p_Value++;

        return p_Value;
      }

    } // namespace Util
  } // namespace Math
} // namespace Low
