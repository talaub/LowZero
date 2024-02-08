#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererInterface.h"
#include "LowRendererResourceRegistry.h"
#include "LowRendererComputeStepConfig.h"
#include "LowRendererComputePipeline.h"
#include "LowRendererRenderFlow.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API ComputeStepData
    {
      Util::Map<RenderFlow, ResourceRegistry> resources;
      ComputeStepConfig config;
      Util::Map<RenderFlow, Util::List<Interface::ComputePipeline>>
          pipelines;
      Util::Map<RenderFlow,
                Util::List<Interface::PipelineResourceSignature>>
          signatures;
      Interface::Context context;
      Resource::Image output_image;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(ComputeStepData);
      }
    };

    struct LOW_RENDERER_API ComputeStep : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<ComputeStep> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      ComputeStep();
      ComputeStep(uint64_t p_Id);
      ComputeStep(ComputeStep &p_Copy);

    private:
      static ComputeStep make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit ComputeStep(const ComputeStep &p_Copy)
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
      static ComputeStep *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static ComputeStep find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      static ComputeStep find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle deserialize(Low::Util::Yaml::Node &p_Node,
                                           Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.get_type() == ComputeStep::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        ComputeStep l_ComputeStep = p_Handle.get_id();
        l_ComputeStep.destroy();
      }

      Util::Map<RenderFlow, ResourceRegistry> &get_resources() const;

      ComputeStepConfig get_config() const;

      Util::Map<RenderFlow, Util::List<Interface::ComputePipeline>> &
      get_pipelines() const;

      Util::Map<RenderFlow,
                Util::List<Interface::PipelineResourceSignature>> &
      get_signatures() const;

      Interface::Context get_context() const;

      Resource::Image get_output_image() const;
      void set_output_image(Resource::Image p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static ComputeStep make(Util::Name p_Name,
                              Interface::Context p_Context,
                              ComputeStepConfig p_Config);
      void prepare(RenderFlow p_RenderFlow);
      void execute(RenderFlow p_RenderFlow);
      void update_dimensions(RenderFlow p_RenderFlow);
      static void create_pipelines(ComputeStep p_Step,
                                   RenderFlow p_RenderFlow);
      static void create_signatures(ComputeStep p_Step,
                                    RenderFlow p_RenderFlow);
      static void prepare_signatures(ComputeStep p_Step,
                                     RenderFlow p_RenderFlow);
      static void default_execute(ComputeStep p_Step,
                                  RenderFlow p_RenderFlow);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_config(ComputeStepConfig p_Value);
      void set_context(Interface::Context p_Value);
    };
  } // namespace Renderer
} // namespace Low
