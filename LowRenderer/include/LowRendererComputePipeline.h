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
      struct PipelineComputeCreateParams;
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT ComputePipelineData
      {
        Backend::Pipeline pipeline;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(ComputePipelineData);
        }
      };

      struct LOW_EXPORT ComputePipeline : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<ComputePipeline> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        ComputePipeline();
        ComputePipeline(uint64_t p_Id);
        ComputePipeline(ComputePipeline &p_Copy);

      private:
        static ComputePipeline make(Low::Util::Name p_Name);

      public:
        explicit ComputePipeline(const ComputePipeline &p_Copy)
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
        static ComputePipeline *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Backend::Pipeline &get_pipeline() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static ComputePipeline make(Util::Name p_Name,
                                    PipelineComputeCreateParams &p_Params);
        void bind();
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
