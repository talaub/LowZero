#pragma once

#include "LowUtilApi.h"

#include <stdint.h>

#include "LowUtilName.h"
#include "LowUtilAssert.h"
#include "LowUtilContainers.h"

#define SINGLE_ARG(...) __VA_ARGS__

#define TYPE_SOA(type, member, membertype)                                     \
  *((membertype *)&(                                                           \
      ms_Buffer[offsetof(##type##Data, member) * type::get_capacity() +        \
                (m_Data.m_Index * sizeof(membertype))]))

#define ACCESSOR_TYPE_SOA(accessor, type, member, membertype)                  \
  *((membertype *)&(                                                           \
      ms_Buffer[offsetof(##type##Data, member) * type::get_capacity() +        \
                (accessor.m_Data.m_Index * sizeof(membertype))]))

namespace Low {
  namespace Util {
    namespace Instances {
      struct LOW_EXPORT Slot
      {
        bool m_Occupied;
        uint16_t m_Generation;
      };

      LOW_EXPORT void initialize_buffer(uint8_t **p_Buffer,
                                        size_t p_ElementSize,
                                        size_t p_ElementCount,
                                        Low::Util::Instances::Slot **p_Slots);

      LOW_EXPORT uint32_t create_instance(uint8_t *p_Buffer,
                                          Instances::Slot *p_Slots,
                                          uint32_t p_Capacity);

      void initialize();
      void cleanup();

    } // namespace Instances

    struct LOW_EXPORT Handle;

    namespace RTTI {
      namespace PropertyType {
        enum Enum
        {
          UNKNOWN,
          VECTOR2,
          VECTOR3,
          NAME
        };
      }

      struct PropertyInfo
      {
        Util::Name name;
        uint32_t dataOffset;
        uint32_t type;
        void const *(*get)(Handle);
        void (*set)(Handle, const void *);
      };

      struct TypeInfo
      {
        Name name;
        Map<Name, PropertyInfo> properties;
        uint32_t (*get_capacity)();
        bool (*is_alive)(Handle);
      };
    } // namespace RTTI

    struct LOW_EXPORT Handle
    {
      union
      {
        uint64_t m_Id;

        struct
        {
          uint32_t m_Index;
          uint16_t m_Generation;
          uint16_t m_Type;
        } m_Data;
      };

      Handle();
      Handle(uint64_t p_Id);

      bool operator==(const Handle &p_Other) const;
      bool operator!=(const Handle &p_Other) const;
      bool operator<(const Handle &p_Other) const;

      uint64_t get_id() const;

      uint32_t get_index() const;
      uint16_t get_generation() const;
      uint16_t get_type() const;

      bool check_alive(Instances::Slot *p_Slots, uint32_t p_Capacity) const;

      static RTTI::TypeInfo &get_type_info(uint16_t p_TypeId);

    protected:
      static void register_type_info(uint16_t p_TypeId,
                                     RTTI::TypeInfo &p_TypeInfo);
    };

  } // namespace Util
} // namespace Low
