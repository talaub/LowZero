
#include "LowUtilVariant.h"
#include <vcruntime_string.h>

#define VARIANT_DATA_SIZE sizeof(Math::Vector4)

namespace Low {
  namespace Util {
    Variant::Variant() : m_Type(VariantType::Int32)
    {
      m_Int32 = 0;
    }
    Variant::Variant(bool p_Value) : m_Type(VariantType::Bool)
    {
      m_Bool = p_Value;
    }
    Variant::Variant(float p_Value) : m_Type(VariantType::Float)
    {
      m_Float = p_Value;
    }
    Variant::Variant(int32_t p_Value) : m_Type(VariantType::Int32)
    {
      m_Int32 = p_Value;
    }
    Variant::Variant(uint32_t p_Value) : m_Type(VariantType::UInt32)
    {
      m_Uint32 = p_Value;
    }
    Variant::Variant(uint64_t p_Value) : m_Type(VariantType::UInt64)
    {
      m_Uint64 = p_Value;
    }
    Variant::Variant(Math::UVector2 p_Value) : m_Type(VariantType::UVector2)
    {
      m_UVector2 = p_Value;
    }
    Variant::Variant(Math::Vector2 p_Value) : m_Type(VariantType::Vector2)
    {
      m_Vector2 = p_Value;
    }
    Variant::Variant(Math::Vector3 p_Value) : m_Type(VariantType::Vector3)
    {
      m_Vector3 = p_Value;
    }
    Variant::Variant(Math::Vector4 p_Value) : m_Type(VariantType::Vector4)
    {
      m_Vector4 = p_Value;
    }
    Variant::Variant(Math::Quaternion p_Value) : m_Type(VariantType::Quaternion)
    {
      m_Quaternion = p_Value;
    }
    Variant::Variant(Handle p_Value) : m_Type(VariantType::Handle)
    {
      m_Uint64 = p_Value.get_id();
    }
    Variant::Variant(Name p_Value) : m_Type(VariantType::Name)
    {
      m_Uint32 = p_Value.m_Index;
    }

    Variant &Variant::operator=(const bool p_Value)
    {
      m_Type = VariantType::Bool;
      m_Bool = p_Value;

      return *this;
    }
    Variant &Variant::operator=(const float p_Value)
    {
      m_Type = VariantType::Float;
      m_Float = p_Value;

      return *this;
    }
    Variant &Variant::operator=(const int32_t p_Value)
    {
      m_Type = VariantType::Int32;
      m_Int32 = p_Value;

      return *this;
    }
    Variant &Variant::operator=(const uint32_t p_Value)
    {
      m_Type = VariantType::UInt32;
      m_Uint32 = p_Value;

      return *this;
    }
    Variant &Variant::operator=(const uint64_t p_Value)
    {
      m_Type = VariantType::UInt64;
      m_Uint64 = p_Value;

      return *this;
    }
    Variant &Variant::operator=(const Math::UVector2 p_Value)
    {
      m_Type = VariantType::UVector2;
      m_UVector2 = p_Value;

      return *this;
    }
    Variant &Variant::operator=(const Math::Vector2 p_Value)
    {
      m_Type = VariantType::Vector2;
      m_Vector2 = p_Value;

      return *this;
    }
    Variant &Variant::operator=(const Math::Vector3 p_Value)
    {
      m_Type = VariantType::Vector3;
      m_Vector3 = p_Value;

      return *this;
    }
    Variant &Variant::operator=(const Math::Vector4 p_Value)
    {
      m_Type = VariantType::Vector4;
      m_Vector4 = p_Value;

      return *this;
    }
    Variant &Variant::operator=(const Math::Quaternion p_Value)
    {
      m_Type = VariantType::Quaternion;
      m_Quaternion = p_Value;

      return *this;
    }
    Variant &Variant::operator=(const Name p_Value)
    {
      m_Type = VariantType::Name;
      m_Uint32 = p_Value.m_Index;

      return *this;
    }
    Variant &Variant::operator=(const Variant &p_Value)
    {
      m_Type = p_Value.m_Type;
      memcpy(&m_Bool, &p_Value.m_Bool, VARIANT_DATA_SIZE);

      return *this;
    }

    void Variant::set_handle(Handle p_Value)
    {
      m_Type = VariantType::Handle;
      m_Uint64 = p_Value.get_id();
    }

    Variant Variant::from_handle(Handle p_Value)
    {
      Variant l_Variant;
      l_Variant.set_handle(p_Value);

      return l_Variant;
    }

    Variant::operator bool() const
    {
      return m_Bool;
    }
    Variant::operator float() const
    {
      return m_Float;
    }
    Variant::operator int32_t() const
    {
      return m_Int32;
    }
    Variant::operator uint32_t() const
    {
      return m_Uint32;
    }
    Variant::operator uint64_t() const
    {
      return m_Uint64;
    }
    Variant::operator Math::UVector2() const
    {
      return m_UVector2;
    }
    Variant::operator Math::Vector2() const
    {
      return m_Vector2;
    }
    Variant::operator Math::Vector3() const
    {
      return m_Vector3;
    }
    Variant::operator Math::Vector4() const
    {
      return m_Vector4;
    }
    Variant::operator Math::Quaternion() const
    {
      return m_Quaternion;
    }
    Variant::operator Name() const
    {
      return Name(m_Uint32);
    }
    Variant::operator Handle() const
    {
      return m_Uint64;
    }

    bool Variant::operator!=(const Variant &p_Other) const
    {
      return !(*this == p_Other);
    }

    bool Variant::operator==(const Variant &p_Other) const
    {
      if (p_Other.m_Type != m_Type) {
        return false;
      }

      if (m_Type == VariantType::Bool) {
        return m_Bool == p_Other.m_Bool;
      }
      if (m_Type == VariantType::Int32) {
        return m_Int32 == p_Other.m_Int32;
      }
      if (m_Type == VariantType::UInt32) {
        return m_Uint32 == p_Other.m_Uint32;
      }
      if (m_Type == VariantType::UInt64) {
        return m_Uint64 == p_Other.m_Uint64;
      }
      if (m_Type == VariantType::Float) {
        return m_Float == p_Other.m_Float;
      }
      if (m_Type == VariantType::UVector2) {
        return m_UVector2 == p_Other.m_UVector2;
      }
      if (m_Type == VariantType::Vector2) {
        return m_Vector2 == p_Other.m_Vector2;
      }
      if (m_Type == VariantType::Vector3) {
        return m_Vector3 == p_Other.m_Vector3;
      }
      if (m_Type == VariantType::Vector4) {
        return m_Vector4 == p_Other.m_Vector4;
      }
      if (m_Type == VariantType::Quaternion) {
        return m_Quaternion == p_Other.m_Quaternion;
      }
      if (m_Type == VariantType::Name) {
        return m_Uint32 == p_Other.m_Uint32;
      }
      if (m_Type == VariantType::Handle) {
        return m_Uint64 == p_Other.m_Uint64;
      }

      LOW_ASSERT(false, "Unknown variant type on == comparison");
      return false;
    }
  } // namespace Util
} // namespace Low
