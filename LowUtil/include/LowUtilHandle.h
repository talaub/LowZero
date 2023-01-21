#pragma once

#include "LowUtilApi.h"

#include <stdint.h>

#define TYPE_SOA(type, member, membertype)                                     \
  *((membertype *)&(ms_Buffer[offsetof(##type##Data, member) +                 \
                              (m_Data.m_Index * ##type##Data::get_size())]))

#define ACCESSOR_TYPE_SOA(accessor, type, member, membertype)                  \
  *((membertype *)&(                                                           \
      ms_Buffer[offsetof(##type##Data, member) +                               \
                (accessor.m_Data.m_Index * ##type##Data::get_size())]))

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
    };

  } // namespace Util
} // namespace Low
