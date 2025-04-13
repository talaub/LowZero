#include "LowRendererVulkan.h"
#include "LowUtilAssert.h"

#include "LowRendererVulkanBuffer.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      bool DynamicBuffer::reserve(u32 p_ElementCount, u32 *p_StartOut)
      {
        if (m_FreeSlots.empty()) {
          *p_StartOut = 0;
          return false;
        }

        DynamicBufferFreeSlot l_FreeSlot{0, 0};
        uint32_t i_SlotIndex = 0;

        // Look for a free slot that would fit our requested size
        for (; i_SlotIndex < m_FreeSlots.size(); ++i_SlotIndex) {
          if (m_FreeSlots[i_SlotIndex].length >= p_ElementCount) {
            l_FreeSlot = m_FreeSlots[i_SlotIndex];
            break;
          }
        }

        // Check if the found free slot is big enough
        if (l_FreeSlot.length < p_ElementCount) {
          *p_StartOut = 0;
          return false;
        }

        *p_StartOut = l_FreeSlot.start;

        // Adjust the free slots
        uint32_t l_SavePoint = l_FreeSlot.start;
        if (l_FreeSlot.length == p_ElementCount) {
          m_FreeSlots.erase(m_FreeSlots.begin() + i_SlotIndex);
        } else {
          m_FreeSlots[i_SlotIndex].length =
              l_FreeSlot.length - p_ElementCount;
          m_FreeSlots[i_SlotIndex].start =
              l_SavePoint + p_ElementCount;
        }

        return true;
      }

      void DynamicBuffer::free(uint32_t p_Position,
                               uint32_t p_ElementCount)
      {
        uint32_t l_ClosestUnder = ~0u;
        uint32_t l_ClosestOver = ~0u;
        uint32_t l_UnderDiff = ~0u;
        uint32_t l_OverDiff = ~0u;

        for (uint32_t i = 0u; i < m_FreeSlots.size(); ++i) {
          DynamicBufferFreeSlot &i_Slot = m_FreeSlots[i];

          if (i_Slot.start < p_Position) {
            LOW_ASSERT((i_Slot.start + i_Slot.length) <= p_Position,
                       "Tried to double free from mesh buffer");

            uint32_t i_Diff =
                p_Position - (i_Slot.start + i_Slot.length);
            if (i_Diff < l_UnderDiff) {
              l_UnderDiff = i_Diff;
              l_ClosestUnder = i;
            }
          } else {
            LOW_ASSERT((p_Position + p_ElementCount) <= i_Slot.start,
                       "Tried to double free from mesh buffer");

            uint32_t i_Diff =
                i_Slot.start - (p_Position + p_ElementCount);
            if (i_Diff < l_OverDiff) {
              l_OverDiff = i_Diff;
              l_ClosestOver = i;
            }
          }
        }

        if (l_UnderDiff == 0 && l_OverDiff == 0) {
          m_FreeSlots[l_ClosestUnder].length =
              m_FreeSlots[l_ClosestUnder].length + p_ElementCount +
              m_FreeSlots[l_ClosestOver].length;

          m_FreeSlots.erase(m_FreeSlots.begin() + l_ClosestOver);
        } else if (l_UnderDiff == 0) {
          m_FreeSlots[l_ClosestUnder].length =
              m_FreeSlots[l_ClosestUnder].length + p_ElementCount;
        } else if (l_OverDiff == 0) {
          m_FreeSlots[l_ClosestOver].start = p_Position;
          m_FreeSlots[l_ClosestOver].length =
              m_FreeSlots[l_ClosestOver].length + p_ElementCount;
        } else {
          m_FreeSlots.push_back({p_Position, p_ElementCount});
        }
      }

      void DynamicBuffer::clear()
      {
        m_FreeSlots.clear();
        m_FreeSlots.push_back({0, m_ElementCount});
      }

      uint32_t DynamicBuffer::get_used_elements() const
      {
        const uint32_t l_MaxElements = m_ElementCount;
        uint32_t l_FreeElements = 0;

        for (auto it = m_FreeSlots.begin(); it != m_FreeSlots.end();
             ++it) {
          l_FreeElements += it->length;
        }

        return l_MaxElements - l_FreeElements;
      }

      void DynamicBuffer::destroy()
      {
        clear();
        BufferUtil::destroy_buffer(m_Buffer);
      }
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
