#pragma once

#include "LowMathApi.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <stdint.h>

#define LOW_MATH_EPSILON 1e-6f

#define LOW_UINT64_MAX std::numeric_limits<uint64_t>::max()
#define LOW_UINT64_MIN 0ull
#define LOW_UINT32_MAX std::numeric_limits<uint32_t>::max()
#define LOW_UINT32_MIN 0u
#define LOW_UINT8_MAX std::numeric_limits<uint8_t>::max()
#define LOW_UINT8_MIN 0u
#define LOW_UINT16_MAX std::numeric_limits<uint16_t>::max()
#define LOW_UINT16_MIN 0u
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
#define LOW_MATH_ABS(x) (x < 0 ? -x : x)

#define LOW_MATH_CLAMP(x, lower, upper)                              \
  (((x) < (lower)) ? (lower) : (((x) > (upper)) ? (upper) : (x)))

#define LOW_VECTOR3_UP Low::Math::Vector3(0.0f, 1.0f, 0.0f)
#define LOW_VECTOR3_FRONT Low::Math::Vector3(0.0f, 0.0f, -1.0f)
#define LOW_VECTOR3_RIGHT Low::Math::Vector3(1.0f, 0.0f, 0.0f)

#define LOW_MATRIX4x4_IDENTITY Low::Math::Matrix4x4(1.0f)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t i32;

namespace Low {
  namespace Math {
    typedef glm::vec2 Vector2;
    typedef glm::vec3 Vector3;
    typedef glm::vec4 Vector4;

    typedef glm::vec3 ColorRGB;
    typedef glm::vec4 Color;

    typedef glm::uvec2 UVector2;
    typedef glm::uvec3 UVector3;
    typedef glm::uvec4 UVector4;

    typedef glm::ivec2 IVector2;

    typedef glm::quat Quaternion;

    typedef glm::mat4 Matrix4x4;

    struct Cylinder
    {
      Vector3 position;
      Quaternion rotation;
      float radius;
      float height;
    };

    struct Box
    {
      Vector3 position;
      Quaternion rotation;
      Vector3 halfExtents;
    };

    struct Sphere
    {
      Vector3 position;
      float radius;
    };

    struct Cone
    {
      Vector3 position;
      Quaternion rotation;
      float radius;
      float height;
    };

    enum class ShapeType
    {
      SPHERE,
      BOX,
      CYLINDER,
      CONE
    };

    struct Shape
    {
      ShapeType type;
      union
      {
        Cylinder cylinder;
        Box box;
        Sphere sphere;
        Cone cone;
      };
    };

    struct Bounds
    {
      Vector3 min;
      Vector3 max;
    };

    struct AABB
    {
      Bounds bounds;
      Vector3 center;
    };

    namespace Util {
      LOW_EXPORT float lerp(float p_Start, float p_End,
                            float p_Delta);
      LOW_EXPORT float power2(float p_Power);
      LOW_EXPORT float power(float p_Base, float p_Power);
      LOW_EXPORT float abs(float p_Num);
      LOW_EXPORT float clamp(float p_Num, float p_Low, float p_High);
      LOW_EXPORT uint32_t clamp(uint32_t p_Num, uint32_t p_Low,
                                uint32_t p_High);

      LOW_EXPORT float floor(float p_Num);

      LOW_EXPORT float asin(float p_Value);
      LOW_EXPORT float atan2(float p_Value0, float p_Value1);

      LOW_EXPORT float random();
      LOW_EXPORT float random_range(float p_Min, float p_Max);
      LOW_EXPORT float random_range_int(int p_Min, int p_Max);
      LOW_EXPORT bool random_percent(u8 p_Percent);
      LOW_EXPORT bool random_percent(float p_Percent);

      LOW_EXPORT u32 next_power_of_two(u32 p_Value);
    } // namespace Util
  } // namespace Math
} // namespace Low
