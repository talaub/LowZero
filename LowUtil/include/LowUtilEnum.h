#pragma once

#include "LowUtilContainers.h"
#include "LowUtilHandle.h"

#include <initializer_list>
#include <type_traits>

namespace Low {
  namespace Util {
    struct EnumMaskBase
    {
    protected:
      u64 m_Bits[4] = {0, 0, 0, 0};

    public:
      bool has_raw(const u8 p_Value) const
      {
        return ((m_Bits[p_Value >> 6] >> (p_Value & 63)) & 1ull) != 0;
      }

      void set_raw(const u8 p_Value)
      {
        m_Bits[p_Value >> 6] |= (1ull << (p_Value & 63));
      }

      void unset_raw(const u8 p_Value)
      {
        m_Bits[p_Value >> 6] &= ~(1ull << (p_Value & 63));
      }

      void set_raw(const u8 p_Value, const bool p_State)
      {
        if (p_State) {
          set_raw(p_Value);
        } else {
          unset_raw(p_Value);
        }
      }

      void clear()
      {
        for (int i = 0; i < 4; ++i) {
          m_Bits[i] = 0;
        }
      }

      bool is_empty() const
      {
        for (int i = 0; i < 4; ++i) {
          if (m_Bits[i] != 0) {
            return false;
          }
        }
        return true;
      }

      bool has_any(const EnumMaskBase &p_Other) const
      {
        for (int i = 0; i < 4; ++i) {
          if ((m_Bits[i] & p_Other.m_Bits[i]) != 0) {
            return true;
          }
        }
        return false;
      }

      bool has_all(const EnumMaskBase &p_Other) const
      {
        for (int i = 0; i < 4; ++i) {
          if ((m_Bits[i] & p_Other.m_Bits[i]) != p_Other.m_Bits[i]) {
            return false;
          }
        }
        return true;
      }

      u32 count() const
      {
        u32 l_Count = 0;
        for (int i = 0; i < 4; ++i) {
          u64 i_Bits = m_Bits[i];
          while (i_Bits != 0) {
            i_Bits &= i_Bits - 1;
            ++l_Count;
          }
        }
        return l_Count;
      }

      void set_all(const u16 p_EnumId)
      {
        RTTI::EnumInfo &l_EnumInfo = get_enum_info(p_EnumId);
        for (const RTTI::EnumEntryInfo &i_Entry :
             l_EnumInfo.entries) {
          set_raw(i_Entry.value);
        }
      }

      void invert(const u16 p_EnumId)
      {
        RTTI::EnumInfo &l_EnumInfo = get_enum_info(p_EnumId);
        for (const RTTI::EnumEntryInfo &i_Entry :
             l_EnumInfo.entries) {
          set_raw(i_Entry.value, !has_raw(i_Entry.value));
        }
      }

      bool operator==(const EnumMaskBase &p_Other) const
      {
        for (int i = 0; i < 4; ++i) {
          if (m_Bits[i] != p_Other.m_Bits[i]) {
            return false;
          }
        }
        return true;
      }

      bool operator!=(const EnumMaskBase &p_Other) const
      {
        return !(*this == p_Other);
      }
    };

    template <typename T> struct EnumMask : public EnumMaskBase
    {
      static_assert(std::is_enum<T>::value,
                    "EnumMask can only be used with enum types");

    private:
      static u8 raw(const T p_Value)
      {
        return static_cast<u8>(p_Value);
      }

    public:
      EnumMask() = default;

      EnumMask(std::initializer_list<T> p_Values)
      {
        for (const T i_Value : p_Values) {
          set(i_Value);
        }
      }

      bool has(const T p_Value) const
      {
        return has_raw(raw(p_Value));
      }

      void set(const T p_Value)
      {
        set_raw(raw(p_Value));
      }

      void set(const T p_Value, const bool p_State)
      {
        set_raw(raw(p_Value), p_State);
      }

      void unset(const T p_Value)
      {
        unset_raw(raw(p_Value));
      }
    };

    static_assert(sizeof(EnumMaskBase) == 32,
                  "EnumMaskBase is expected to be 256 bits");
  } // namespace Util
} // namespace Low
