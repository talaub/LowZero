#pragma once

#include "LowMathApi.h"

#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

#include <stdint.h>

#define LOW_UINT64_MAX std::numeric_limits<uint64_t>::max()
#define LOW_UINT64_MIN 0ull
#define LOW_UINT32_MAX std::numeric_limits<uint32_t>::max()
#define LOW_UINT32_MIN 0u
#define LOW_UINT8_MAX std::numeric_limits<uint8_t>::max()
#define LOW_UINT8_MIN 0u
#define LOW_SHORT_MAX std::numeric_limits<short>::max()
#define LOW_SHORT_MIN std::numeric_limits<short>::min()
#define LOW_INT_MAX std::numeric_limits<int>::max()
#define LOW_INT_MIN std::numeric_limits<int>::min()
#define LOW_LONG_MAX std::numeric_limits<long>::max()
#define LOW_LONG_MIN std::numeric_limits<long>::min()
#define LOW_LLONG_MAX std::numeric_limits<long long>::max()
#define LOW_LLONG_MIN std::numeric_limits<long long>::min()
#define LOW_FLOAT_MAX std::numeric_limits<float>::max()
#define LOW_FLOAT_MIN std::numeric_limits<float>::min()
#define LOW_DOUBLE_MAX std::numeric_limits<double>::max()
#define LOW_DOUBLE_MIN std::numeric_limits<double>::min()
#define LOW_ULLONG_MIN 0ull
#define LOW_ULLONG_MAX std::numeric_limits<unsigned long long>::max()

#define LOW_KILOBYTE_F 1024.0f
#define LOW_MEGABYTE_F (1024.0f * 1024.0f)
#define LOW_GIGABYTE_F (1024.f * 1024.f * 1024.f)

#define LOW_KILOBYTE_I 1024
#define LOW_MEGABYTE_I (1024 * 1024)
#define LOW_GIGABYTE_I (1024 * 1024 * 1024)

#define LOW_MATH_MAX(x, y) (x > y ? x : y)
#define LOW_MATH_MIN(x, y) (x < y ? x : y)

namespace Low {
  namespace Math {
    typedef glm::vec2 Vector2;
    typedef glm::vec3 Vector3;
    typedef glm::vec4 Vector4;

    typedef glm::vec3 ColorRGB;
    typedef glm::vec4 Color;

    typedef glm::uvec2 UVector2;

    typedef glm::quat Quaternion;

    typedef glm::mat4 Matrix4x4;

    namespace Util {
      LOW_EXPORT float lerp(float p_Start, float p_End, float p_Delta);
      LOW_EXPORT float power2(float p_Power);
      LOW_EXPORT float power(float p_Base, float p_Power);
      LOW_EXPORT float abs(float p_Num);
      LOW_EXPORT float clamp(float p_Num, float p_Low, float p_High);
      LOW_EXPORT uint32_t clamp(uint32_t p_Num, uint32_t p_Low,
                                uint32_t p_High);
    } // namespace Util
  }   // namespace Math
} // namespace Low
