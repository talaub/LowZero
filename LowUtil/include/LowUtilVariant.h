#pragma once

#include "LowUtilApi.h"

#include "LowMath.h"

#include "LowUtilHandle.h"
#include "LowUtilContainers.h"

#include <type_traits>

namespace Low {
  namespace Util {
    namespace VariantType {
      enum Enum
      {
        Bool,
        Int32,
        UInt32,
        UInt64,
        Float,
        UVector2,
        Vector2,
        Vector3,
        Vector4,
        Quaternion,
        Name,
        Handle,
        String
      };
    }

    struct LOW_EXPORT Variant
    {
      uint8_t m_Type;
      union
      {
        bool m_Bool;
        float m_Float;
        int32_t m_Int32;
        uint32_t m_Uint32;
        uint64_t m_Uint64;
        Math::UVector2 m_UVector2;
        Math::Vector2 m_Vector2;
        Math::Vector3 m_Vector3;
        Math::Vector4 m_Vector4;
        Math::Quaternion m_Quaternion;
      };

      Variant();
      Variant(bool p_Value);
      Variant(float p_Value);
      Variant(int32_t p_Value);
      Variant(uint32_t p_Value);
      Variant(uint64_t p_Value);
      Variant(Math::UVector2 p_Value);
      Variant(Math::Vector2 p_Value);
      Variant(Math::Vector3 p_Value);
      Variant(Math::Vector4 p_Value);
      Variant(Math::Quaternion p_Value);
      Variant(Handle p_Value);
      Variant(Name p_Value);

      Variant &operator=(const bool p_Value);
      Variant &operator=(const float p_Value);
      Variant &operator=(const int32_t p_Value);
      Variant &operator=(const uint32_t p_Value);
      Variant &operator=(const uint64_t p_Value);
      Variant &operator=(const Math::UVector2 p_Value);
      Variant &operator=(const Math::Vector2 p_Value);
      Variant &operator=(const Math::Vector3 p_Value);
      Variant &operator=(const Math::Vector4 p_Value);
      Variant &operator=(const Math::Quaternion p_Value);
      Variant &operator=(const Name p_Value);
      Variant &operator=(const Handle p_Value);
      Variant &operator=(const Variant &p_Value);

      bool operator==(const Variant &p_Other) const;
      bool operator!=(const Variant &p_Other) const;

      void set_handle(Handle p_Value);

      static Variant from_handle(Handle p_Value);

      operator bool() const;
      operator float() const;
      operator int32_t() const;
      operator uint32_t() const;
      operator uint64_t() const;
      operator Math::UVector2() const;
      operator Math::Vector2() const;
      operator Math::Vector3() const;
      operator Math::Vector4() const;
      operator Math::Quaternion() const;
      operator Handle() const;
      operator Name() const;
    };
  } // namespace Util
} // namespace Low
