#pragma once

#include "LowUtilApi.h"

#include "LowMath.h"

#include "LowUtilHandle.h"

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
        Vector2,
        Vector3,
        Vector4,
        Quaternion,
        Handle
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
      Variant(Math::Vector2 p_Value);
      Variant(Math::Vector3 p_Value);
      Variant(Math::Vector4 p_Value);
      Variant(Math::Quaternion p_Value);
      Variant(Handle p_Value);

      Variant &Variant::operator=(bool p_Value);
      Variant &Variant::operator=(float p_Value);
      Variant &Variant::operator=(int32_t p_Value);
      Variant &Variant::operator=(uint32_t p_Value);
      Variant &Variant::operator=(uint64_t p_Value);
      Variant &Variant::operator=(Math::Vector2 p_Value);
      Variant &Variant::operator=(Math::Vector3 p_Value);
      Variant &Variant::operator=(Math::Vector4 p_Value);
      Variant &Variant::operator=(Math::Quaternion p_Value);
      Variant &Variant::operator=(Handle p_Value);

      void set_handle(Handle &p_Value);

      static Variant from_handle(Handle &p_Value);

      operator bool() const;
      operator float() const;
      operator int32_t() const;
      operator uint32_t() const;
      operator uint64_t() const;
      operator Math::Vector2() const;
      operator Math::Vector3() const;
      operator Math::Vector4() const;
      operator Math::Quaternion() const;
      operator Handle() const;
    };
  } // namespace Util
} // namespace Low
