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

      struct LOW_EXPORT CommandPoolData
      {
        Low::Renderer::Backend::CommandPool commandpool;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(CommandPoolData);
        }
      };

      struct LOW_EXPORT CommandPool : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<CommandPool> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        CommandPool();
        CommandPool(uint64_t p_Id);
        CommandPool(CommandPool &p_Copy);

        static CommandPool make(Low::Util::Name p_Name);
        explicit CommandPool(const CommandPool &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void cleanup();

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static CommandPool *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Low::Renderer::Backend::CommandPool &get_commandpool() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
