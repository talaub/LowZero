#include "LowUtilHandle.h"

#include "LowUtilAssert.h"
#include "LowUtilContainers.h"

#include <stdlib.h>

namespace Low {
  namespace Util {
    Handle::Handle()
    {
    }

    Handle::Handle(uint64_t p_Id) : m_Id(p_Id)
    {
    }

    bool Handle::operator==(const Handle &p_Other) const
    {
      return m_Id == p_Other.m_Id;
    }

    bool Handle::operator!=(const Handle &p_Other) const
    {
      return m_Id != p_Other.m_Id;
    }

    bool Handle::operator<(const Handle &p_Other) const
    {
      return m_Id < p_Other.m_Id;
    }

    uint64_t Handle::get_id() const
    {
      return m_Id;
    }

    uint32_t Handle::get_index() const
    {
      return m_Data.m_Index;
    }

    uint16_t Handle::get_generation() const
    {
      return m_Data.m_Generation;
    }

    uint16_t Handle::get_type() const
    {
      return m_Data.m_Type;
    }

    bool Handle::check_alive(Instances::Slot *p_Slots,
                             uint32_t p_Capacity) const
    {
      if (m_Data.m_Index >= p_Capacity) {
        return false;
      }

      return p_Slots[m_Data.m_Index].m_Occupied &&
             p_Slots[m_Data.m_Index].m_Generation == m_Data.m_Generation;
    }

    namespace Instances {

      void initialize_buffer(uint8_t **p_Buffer, size_t p_ElementSize,
                             size_t p_ElementCount,
                             Low::Util::Instances::Slot **p_Slots)
      {
        void *l_Buffer =
            calloc(p_ElementCount * p_ElementSize, sizeof(uint8_t));
        (*p_Buffer) = (uint8_t *)l_Buffer;

        void *l_SlotBuffer =
            malloc(p_ElementCount * sizeof(Low::Util::Instances::Slot));
        (*p_Slots) = (Low::Util::Instances::Slot *)l_SlotBuffer;

        // Initialize slots with unoccupied
        for (int i_Iter = 0; i_Iter < p_ElementCount; i_Iter++) {
          (*p_Slots)[i_Iter].m_Occupied = false;
          (*p_Slots)[i_Iter].m_Generation = 0;
        }
      }

    uint32_t create_instance(uint8_t *p_Buffer, Instances::Slot *p_Slots,
			    uint32_t p_Capacity)
    {
    uint32_t l_Index = 0u;

    // Find free index
    for (; l_Index < p_Capacity; ++l_Index) {
	if (!p_Slots[l_Index].m_Occupied &&
	    p_Slots[l_Index].m_Generation <= 255)
	break;
    }

    LOW_ASSERT(l_Index < p_Capacity,
		"Cannot create new instance. Budget blown.");

    p_Slots[l_Index].m_Occupied = true;
    return l_Index;
    }
    } // namespace Instances
  }   // namespace Util
} // namespace Low
