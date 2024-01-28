#pragma once

#include "LowUtilName.h"

#include "LowRendererContext.h"

namespace Low {
  namespace Renderer {

    struct DynamicBufferFreeSlot
    {
      uint32_t start;
      uint32_t length;
    };

    namespace DynamicBufferType {
      enum Enum
      {
        VERTEX,
        INDEX,
        MISC
      };
    }

    struct DynamicBuffer
    {
      void initialize(Util::Name p_Name, Interface::Context p_Context,
                      uint8_t p_Type, uint32_t p_ElementSize,
                      uint32_t p_ElementCount);

      uint32_t reserve(uint32_t p_ElementCount);

      uint32_t write(void *p_DataPtr, uint32_t p_ElementCount);
      void free(uint32_t p_Position, uint32_t p_ElementCount);

      void clear();
      void bind();

      uint32_t get_used_elements() const;

      Resource::Buffer m_Buffer;

    private:
      uint32_t m_ElementSize;
      uint32_t m_ElementCount;
      Util::List<DynamicBufferFreeSlot> m_FreeSlots;
      bool m_Initialized = false;
      uint8_t m_Type;
    };
  } // namespace Renderer
} // namespace Low
