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
      struct UniformPoolCreateParams;
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT UniformPoolData
      {
        Backend::UniformPool pool;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(UniformPoolData);
        }
      };

      struct LOW_EXPORT UniformPool : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<UniformPool> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        UniformPool();
        UniformPool(uint64_t p_Id);
        UniformPool(UniformPool &p_Copy);

        static UniformPool make(Low::Util::Name p_Name);
        explicit UniformPool(const UniformPool &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static UniformPool *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Backend::UniformPool &get_pool() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static UniformPool make(Util::Name p_Name,
                                UniformPoolCreateParams &p_Params);
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
