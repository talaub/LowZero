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
      struct UniformBufferCreateParams;
      struct UniformBufferSetParams;
      struct UniformImageCreateParams;
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT UniformData
      {
        Backend::Uniform uniform;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(UniformData);
        }
      };

      struct LOW_EXPORT Uniform : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<Uniform> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        Uniform();
        Uniform(uint64_t p_Id);
        Uniform(Uniform &p_Copy);

      private:
        static Uniform make(Low::Util::Name p_Name);

      public:
        explicit Uniform(const Uniform &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static Uniform *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Backend::Uniform &get_uniform() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Uniform make_buffer(Util::Name p_Name,
                                   UniformBufferCreateParams &p_Params);
        static Uniform make_image(Util::Name p_Name,
                                  UniformImageCreateParams &p_Params);
        void set_buffer_initial(void *p_Value);
        void set_buffer_frame(UniformBufferSetParams &p_Params);
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
