#include "LowUtilVariant.h"

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

    Variant &Variant::operator=(bool p_Value)
    {
      m_Type = VariantType::Bool;
      m_Bool = p_Value;

      return *this;
    }
    Variant &Variant::operator=(float p_Value)
    {
      m_Type = VariantType::Float;
      m_Float = p_Value;

      return *this;
    }
    Variant &Variant::operator=(int32_t p_Value)
    {
      m_Type = VariantType::Int32;
      m_Int32 = p_Value;

      return *this;
    }
    Variant &Variant::operator=(uint32_t p_Value)
    {
      m_Type = VariantType::UInt32;
      m_Uint32 = p_Value;

      return *this;
    }
    Variant &Variant::operator=(uint64_t p_Value)
    {
      m_Type = VariantType::UInt64;
      m_Uint64 = p_Value;

      return *this;
    }
    Variant &Variant::operator=(Math::Vector2 p_Value)
    {
      m_Type = VariantType::Vector2;
      m_Vector2 = p_Value;

      return *this;
    }
    Variant &Variant::operator=(Math::Vector3 p_Value)
    {
      m_Type = VariantType::Vector3;
      m_Vector3 = p_Value;

      return *this;
    }
    Variant &Variant::operator=(Math::Vector4 p_Value)
    {
      m_Type = VariantType::Vector4;
      m_Vector4 = p_Value;

      return *this;
    }
    Variant &Variant::operator=(Math::Quaternion p_Value)
    {
      m_Type = VariantType::Quaternion;
      m_Quaternion = p_Value;

      return *this;
    }

    void Variant::set_handle(Handle &p_Value)
    {
      m_Type = VariantType::Handle;
      m_Uint64 = p_Value.get_id();
    }

    Variant Variant::from_handle(Handle &p_Value)
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
    Variant::operator Handle() const
    {
      return m_Uint64;
    }
  } // namespace Util
} // namespace Low
