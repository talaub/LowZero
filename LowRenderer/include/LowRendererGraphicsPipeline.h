#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererBackend.h"

namespace Low {
  namespace Renderer {
    namespace Backend {
      struct LOW_EXPORT GraphicsPipelineData
      {
        Low::Renderer::Backend::Pipeline pipeline;
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

        static GraphicsPipeline make(Low::Util::Name p_Name);
        void destroy();

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

        Low::Renderer::Backend::Pipeline &get_pipeline() const;
        void set_pipeline(Low::Renderer::Backend::Pipeline &p_Value);

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);
      };
    } // namespace Backend
  }   // namespace Renderer
} // namespace Low
