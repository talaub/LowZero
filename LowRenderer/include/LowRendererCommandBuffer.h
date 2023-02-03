#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererBackend.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT CommandBufferData
      {
        Low::Renderer::Backend::CommandBuffer commandbuffer;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(CommandBufferData);
        }
      };

      struct LOW_EXPORT CommandBuffer : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<CommandBuffer> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        CommandBuffer();
        CommandBuffer(uint64_t p_Id);
        CommandBuffer(CommandBuffer &p_Copy);

        static CommandBuffer make(Low::Util::Name p_Name);
        explicit CommandBuffer(const CommandBuffer &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void cleanup();

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static CommandBuffer *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Low::Renderer::Backend::CommandBuffer &get_commandbuffer() const;
        void set_commandbuffer(Low::Renderer::Backend::CommandBuffer &p_Value);

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
