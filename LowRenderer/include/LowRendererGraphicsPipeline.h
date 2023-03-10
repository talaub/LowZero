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
      struct PipelineGraphicsCreateParams;
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT GraphicsPipelineData
      {
        Backend::Pipeline pipeline;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(GraphicsPipelineData);
        }
      };

      struct LOW_EXPORT GraphicsPipeline : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<GraphicsPipeline> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        GraphicsPipeline();
        GraphicsPipeline(uint64_t p_Id);
        GraphicsPipeline(GraphicsPipeline &p_Copy);

      private:
        static GraphicsPipeline make(Low::Util::Name p_Name);

      public:
        explicit GraphicsPipeline(const GraphicsPipeline &p_Copy)
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
        static GraphicsPipeline *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Backend::Pipeline &get_pipeline() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static GraphicsPipeline make(Util::Name p_Name,
                                     PipelineGraphicsCreateParams &p_Params);
        void bind();
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
