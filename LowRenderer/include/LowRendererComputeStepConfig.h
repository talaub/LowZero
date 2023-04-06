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
    struct LOW_RENDERER_API ComputeStep;

    struct ComputeStepCallbacks
    {
      void (*setup_signatures)(ComputeStep, RenderFlow);
      void (*setup_pipelines)(ComputeStep, RenderFlow);
      void (*populate_signatures)(ComputeStep, RenderFlow);
      void (*execute)(ComputeStep, RenderFlow);
    };
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API ComputeStepConfigData
    {
      ComputeStepCallbacks callbacks;
      Util::List<ResourceConfig> resources;
      Util::List<ComputePipelineConfig> pipelines;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(ComputeStepConfigData);
      }
    };

    struct LOW_RENDERER_API ComputeStepConfig : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<ComputeStepConfig> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      ComputeStepConfig();
      ComputeStepConfig(uint64_t p_Id);
      ComputeStepConfig(ComputeStepConfig &p_Copy);

    private:
      static ComputeStepConfig make(Low::Util::Name p_Name);

    public:
      explicit ComputeStepConfig(const ComputeStepConfig &p_Copy)
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
      static ComputeStepConfig *living_instances()
      {
        return ms_LivingInstances.data();
      }

      bool is_alive() const;

      static uint32_t get_capacity();

      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.check_alive(ms_Slots, get_capacity());
      }

      ComputeStepCallbacks &get_callbacks() const;
      void set_callbacks(ComputeStepCallbacks &p_Value);

      Util::List<ResourceConfig> &get_resources() const;

      Util::List<ComputePipelineConfig> &get_pipelines() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static ComputeStepConfig make(Util::Name p_Name,
                                    Util::Yaml::Node &p_Node);
    };
  } // namespace Renderer
} // namespace Low
