#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererFrontendConfig.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_EXPORT GraphicsStepConfigData
    {
      Util::List<ResourceConfig> resources;
      Util::List<GraphicsPipelineConfig> pipelines;
      Util::List<PipelineResourceBindingConfig> rendertargets;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(GraphicsStepConfigData);
      }
    };

    struct LOW_EXPORT GraphicsStepConfig : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<GraphicsStepConfig> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      GraphicsStepConfig();
      GraphicsStepConfig(uint64_t p_Id);
      GraphicsStepConfig(GraphicsStepConfig &p_Copy);

    private:
      static GraphicsStepConfig make(Low::Util::Name p_Name);

    public:
      explicit GraphicsStepConfig(const GraphicsStepConfig &p_Copy)
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
      static GraphicsStepConfig *living_instances()
      {
        return ms_LivingInstances.data();
      }

      bool is_alive() const;

      static uint32_t get_capacity();

      Util::List<ResourceConfig> &get_resources() const;

      Util::List<GraphicsPipelineConfig> &get_pipelines() const;

      Util::List<PipelineResourceBindingConfig> &get_rendertargets() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static GraphicsStepConfig make(Util::Name p_Name,
                                     Util::Yaml::Node &p_Node);
    };
  } // namespace Renderer
} // namespace Low