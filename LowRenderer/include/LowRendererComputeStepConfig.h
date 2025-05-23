#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererFrontendConfig.h"

#include "shared_mutex"
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
      PipelineResourceBindingConfig output_image;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(ComputeStepConfigData);
      }
    };

    struct LOW_RENDERER_API ComputeStepConfig
        : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<ComputeStepConfig> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      ComputeStepConfig();
      ComputeStepConfig(uint64_t p_Id);
      ComputeStepConfig(ComputeStepConfig &p_Copy);

      static ComputeStepConfig make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
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

      static ComputeStepConfig find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      ComputeStepConfig duplicate(Low::Util::Name p_Name) const;
      static ComputeStepConfig duplicate(ComputeStepConfig p_Handle,
                                         Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static ComputeStepConfig find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == ComputeStepConfig::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        ComputeStepConfig l_ComputeStepConfig = p_Handle.get_id();
        l_ComputeStepConfig.destroy();
      }

      ComputeStepCallbacks &get_callbacks() const;
      void set_callbacks(ComputeStepCallbacks &p_Value);

      Util::List<ResourceConfig> &get_resources() const;

      Util::List<ComputePipelineConfig> &get_pipelines() const;

      PipelineResourceBindingConfig &get_output_image() const;
      void set_output_image(PipelineResourceBindingConfig &p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static ComputeStepConfig make(Util::Name p_Name,
                                    Util::Yaml::Node &p_Node);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
