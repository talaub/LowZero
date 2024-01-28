#include "LowRendererDynamicBuffer.h"

namespace Low {
  namespace Renderer {
    void DynamicBuffer::initialize(Util::Name p_Name,
                                   Interface::Context p_Context, uint8_t p_Type,
                                   uint32_t p_ElementSize,
                                   uint32_t p_ElementCount)
    {
      LOW_ASSERT(!m_Initialized, "DynamicBuffer already initialized");

      m_ElementSize = p_ElementSize;
      m_ElementCount = p_ElementCount;

      Backend::BufferCreateParams l_Params;
      l_Params.context = &p_Context.get_context();
      l_Params.bufferSize = p_ElementSize * p_ElementCount;
      l_Params.data = nullptr;
      if (p_Type == DynamicBufferType::VERTEX) {
        l_Params.usageFlags = LOW_RENDERER_BUFFER_USAGE_VERTEX |
                              LOW_RENDERER_BUFFER_USAGE_RESOURCE_BUFFER;
      } else if (p_Type == DynamicBufferType::INDEX) {
        l_Params.usageFlags = LOW_RENDERER_BUFFER_USAGE_INDEX;
      } else if (p_Type == DynamicBufferType::MISC) {
        l_Params.usageFlags = LOW_RENDERER_BUFFER_USAGE_RESOURCE_BUFFER;
      } else {
        LOW_ASSERT(false, "Unknown mesh buffer type");
      }
      m_Buffer = Resource::Buffer::make(p_Name, l_Params);
      m_FreeSlots.push_back({0, p_ElementCount});

      m_Type = p_Type;

      m_Initialized = true;
    }

    uint32_t DynamicBuffer::reserve(uint32_t p_ElementCount)
    {
      LOW_ASSERT(m_Initialized, "Cannot write to uninitialized Dynamic Buffer");
      LOW_ASSERT(!m_FreeSlots.empty(), "No free space left in Dynamic Buffer");

      DynamicBufferFreeSlot l_FreeSlot{0, 0};
      uint32_t i_SlotIndex = 0;

      for (; i_SlotIndex < m_FreeSlots.size(); ++i_SlotIndex) {
        if (m_FreeSlots[i_SlotIndex].length >= p_ElementCount) {
          l_FreeSlot = m_FreeSlots[i_SlotIndex];
          break;
        }
      }

      LOW_ASSERT(l_FreeSlot.length >= p_ElementCount,
                 "Could not find free space in DynamicBuffer to fit data");

      uint32_t l_SavePoint = l_FreeSlot.start;
      if (l_FreeSlot.length == p_ElementCount) {
        m_FreeSlots.erase(m_FreeSlots.begin() + i_SlotIndex);
      } else {
        m_FreeSlots[i_SlotIndex].length = l_FreeSlot.length - p_ElementCount;
        m_FreeSlots[i_SlotIndex].start = l_SavePoint + p_ElementCount;
      }

      return l_SavePoint;
    }

    uint32_t DynamicBuffer::write(void *p_DataPtr, uint32_t p_ElementCount)
    {
      uint32_t l_Offset = reserve(p_ElementCount);

      m_Buffer.write(p_DataPtr, p_ElementCount * m_ElementSize,
                     l_Offset * m_ElementSize);

      return l_Offset;
    }

    void DynamicBuffer::free(uint32_t p_Position, uint32_t p_ElementCount)
    {
      LOW_ASSERT(m_Initialized, "Cannot free from uninitialized DynamicBuffer");

      uint32_t l_ClosestUnder = ~0u;
      uint32_t l_ClosestOver = ~0u;
      uint32_t l_UnderDiff = ~0u;
      uint32_t l_OverDiff = ~0u;

      for (uint32_t i = 0u; i < m_FreeSlots.size(); ++i) {
        DynamicBufferFreeSlot &i_Slot = m_FreeSlots[i];

        if (i_Slot.start < p_Position) {
          LOW_ASSERT((i_Slot.start + i_Slot.length) <= p_Position,
                     "Tried to double free from mesh buffer");

          uint32_t i_Diff = p_Position - (i_Slot.start + i_Slot.length);
          if (i_Diff < l_UnderDiff) {
            l_UnderDiff = i_Diff;
            l_ClosestUnder = i;
          }
        } else {
          LOW_ASSERT((p_Position + p_ElementCount) <= i_Slot.start,
                     "Tried to double free from mesh buffer");

          uint32_t i_Diff = i_Slot.start - (p_Position + p_ElementCount);
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

    void DynamicBuffer::bind()
    {
      if (m_Type == DynamicBufferType::VERTEX) {
        m_Buffer.bind_vertex();
      } else if (m_Type == DynamicBufferType::INDEX) {
        m_Buffer.bind_index(Backend::IndexBufferType::UINT32);
      } else if (m_Type == DynamicBufferType::MISC) {
        LOW_ASSERT(false, "Cannot implicitly bind misc dynamic buffer");
      } else {
        LOW_ASSERT(false, "Unknown dynamic buffer type");
      }
    }

    uint32_t DynamicBuffer::get_used_elements() const
    {
      const uint32_t l_MaxElements = m_ElementCount;
      uint32_t l_FreeElements = 0;

      for (auto it = m_FreeSlots.begin(); it != m_FreeSlots.end(); ++it) {
        l_FreeElements += it->length;
      }

      return l_MaxElements - l_FreeElements;
    }
  } // namespace Renderer
} // namespace Low
