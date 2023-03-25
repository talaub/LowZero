#include "LowRendererComputeStep.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Renderer {
    const uint16_t ComputeStep::TYPE_ID = 2;
    uint8_t *ComputeStep::ms_Buffer = 0;
    Low::Util::Instances::Slot *ComputeStep::ms_Slots = 0;
    Low::Util::List<ComputeStep> ComputeStep::ms_LivingInstances =
        Low::Util::List<ComputeStep>();

    ComputeStep::ComputeStep() : Low::Util::Handle(0ull)
    {
    }
    ComputeStep::ComputeStep(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    ComputeStep::ComputeStep(ComputeStep &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    ComputeStep ComputeStep::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = Low::Util::Instances::create_instance(
          ms_Buffer, ms_Slots, get_capacity());

      ComputeStepData *l_DataPtr =
          (ComputeStepData *)&ms_Buffer[l_Index * sizeof(ComputeStepData)];
      new (l_DataPtr) ComputeStepData();

      ComputeStep l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = ComputeStep::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, ComputeStep, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      return l_Handle;
    }

    void ComputeStep::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const ComputeStep *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
      _LOW_ASSERT(l_LivingInstanceFound);
    }

    void ComputeStep::initialize()
    {
      initialize_buffer(&ms_Buffer, ComputeStepData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_ComputeStep);
      LOW_PROFILE_ALLOC(type_slots_ComputeStep);
    }

    void ComputeStep::cleanup()
    {
      Low::Util::List<ComputeStep> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_ComputeStep);
      LOW_PROFILE_FREE(type_slots_ComputeStep);
    }

    bool ComputeStep::is_alive() const
    {
      return m_Data.m_Type == ComputeStep::TYPE_ID &&
             check_alive(ms_Slots, ComputeStep::get_capacity());
    }

    uint32_t ComputeStep::get_capacity()
    {
      static uint32_t l_Capacity = 0u;
      if (l_Capacity == 0u) {
        l_Capacity =
            Low::Util::Config::get_capacity(N(LowRenderer), N(ComputeStep));
      }
      return l_Capacity;
    }

    Util::Map<RenderFlow, ResourceRegistry> &ComputeStep::get_resources() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(ComputeStep, resources,
                      SINGLE_ARG(Util::Map<RenderFlow, ResourceRegistry>));
    }

    ComputeStepConfig ComputeStep::get_config() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(ComputeStep, config, ComputeStepConfig);
    }
    void ComputeStep::set_config(ComputeStepConfig p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(ComputeStep, config, ComputeStepConfig) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_config
      // LOW_CODEGEN::END::CUSTOM:SETTER_config
    }

    Util::List<Interface::ComputePipeline> &ComputeStep::get_pipelines() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(ComputeStep, pipelines,
                      Util::List<Interface::ComputePipeline>);
    }

    Util::List<Interface::PipelineResourceSignature> &
    ComputeStep::get_signatures() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(ComputeStep, signatures,
                      Util::List<Interface::PipelineResourceSignature>);
    }

    Interface::Context ComputeStep::get_context() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(ComputeStep, context, Interface::Context);
    }
    void ComputeStep::set_context(Interface::Context p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(ComputeStep, context, Interface::Context) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_context
      // LOW_CODEGEN::END::CUSTOM:SETTER_context
    }

    Low::Util::Name ComputeStep::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(ComputeStep, name, Low::Util::Name);
    }
    void ComputeStep::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(ComputeStep, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    ComputeStep ComputeStep::make(Util::Name p_Name,
                                  Interface::Context p_Context,
                                  ComputeStepConfig p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      ComputeStep l_Step = ComputeStep::make(p_Name);

      l_Step.set_config(p_Config);
      l_Step.set_context(p_Context);

      for (uint32_t i = 0; i < p_Config.get_pipelines().size(); ++i) {
        ComputePipelineConfig &i_Config = p_Config.get_pipelines()[i];

        Util::List<Backend::PipelineResourceDescription> i_ResourceDescriptions;
        for (auto it = i_Config.resourceBinding.begin();
             it != i_Config.resourceBinding.end(); ++it) {
          Backend::PipelineResourceDescription i_Resource;
          i_Resource.name = it->resourceName;
          i_Resource.step = Backend::ResourcePipelineStep::COMPUTE;

          if (it->bindType == ResourceBindType::IMAGE) {
            i_Resource.type = Backend::ResourceType::IMAGE;
          } else if (it->bindType == ResourceBindType::SAMPLER) {
            i_Resource.type = Backend::ResourceType::SAMPLER;
          } else {
            LOW_ASSERT(false, "Unknown resource bind type");
          }

          bool i_Found = false;
          for (auto rit = p_Config.get_resources().begin();
               rit != p_Config.get_resources().end(); ++rit) {
            if (rit->name == it->resourceName) {
              i_Found = true;
              i_Resource.arraySize = rit->arraySize;
              break;
            }
          }
          LOW_ASSERT(i_Found, "Cannot bind resource not found in renderstep");

          i_ResourceDescriptions.push_back(i_Resource);
        }

        l_Step.get_signatures().push_back(
            Interface::PipelineResourceSignature::make(p_Name, p_Context, 1,
                                                       i_ResourceDescriptions));

        Interface::PipelineComputeCreateParams l_Params;
        l_Params.context = p_Context;
        l_Params.shaderPath = i_Config.shader;
        l_Params.signatures = {p_Context.get_global_signature(),
                               l_Step.get_signatures()[i]};
        l_Step.get_pipelines().push_back(
            Interface::ComputePipeline::make(p_Name, l_Params));
      }

      return l_Step;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    void ComputeStep::prepare(RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_prepare
      Util::Map<RenderFlow, ResourceRegistry> &l_Resources = get_resources();
      l_Resources[p_RenderFlow].initialize(get_config().get_resources(),
                                           get_context(), p_RenderFlow);

      Util::List<ComputePipelineConfig> &l_Configs =
          get_config().get_pipelines();
      for (uint32_t i = 0; i < l_Configs.size(); ++i) {
        ComputePipelineConfig &i_Config = l_Configs[i];

        Interface::PipelineResourceSignature i_Signature = get_signatures()[i];
        for (auto it = i_Config.resourceBinding.begin();
             it != i_Config.resourceBinding.end(); ++it) {
          if (it->bindType == ResourceBindType::IMAGE) {
            i_Signature.set_image_resource(
                it->resourceName, 0,
                get_resources()[p_RenderFlow].get_image_resource(
                    it->resourceName));
          } else {
            LOW_ASSERT(false, "Unsupported resource bind type");
          }
        }
      }

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_prepare
    }

    void ComputeStep::execute(RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_execute
      LOW_ASSERT(get_resources().find(p_RenderFlow) != get_resources().end(),
                 "Step not prepared for renderflow");

      if (get_context().get_debug_enabled()) {
        Util::String l_RenderDocLabel =
            Util::String("ComputeStep - ") + get_name().c_str();
        LOW_RENDERER_BEGIN_RENDERDOC_SECTION(
            get_context().get_context(), l_RenderDocLabel,
            Math::Color(0.4249f, 0.2341f, 0.341f, 1.0f));
      }

      Util::List<ComputePipelineConfig> &l_Configs =
          get_config().get_pipelines();
      for (uint32_t i = 0; i < l_Configs.size(); ++i) {
        ComputePipelineConfig &i_Config = l_Configs[i];

        Interface::PipelineResourceSignature i_Signature = get_signatures()[i];
        i_Signature.commit();

        get_pipelines()[i].bind();

        Math::UVector3 i_DispatchDimensions;
        if (i_Config.dispatchConfig.dimensionType ==
            ComputeDispatchDimensionType::ABSOLUTE) {
          i_DispatchDimensions = i_Config.dispatchConfig.absolute;
        } else if (i_Config.dispatchConfig.dimensionType ==
                   ComputeDispatchDimensionType::RELATIVE) {
          Math::UVector2 i_Dimensions;
          if (i_Config.dispatchConfig.relative.target ==
              ComputeDispatchRelativeTarget::RENDERFLOW) {
            i_Dimensions = p_RenderFlow.get_dimensions();
          } else if (i_Config.dispatchConfig.relative.target ==
                     ComputeDispatchRelativeTarget::CONTEXT) {
            i_Dimensions = get_context().get_dimensions();
          } else {
            LOW_ASSERT(false, "Unknown dispatch dimensions relative target");
          }
          Math::Vector2 i_FloatDimensions = i_Dimensions;
          i_FloatDimensions *= i_Config.dispatchConfig.relative.multiplier;
          i_DispatchDimensions.x = i_FloatDimensions.x;
          i_DispatchDimensions.y = i_FloatDimensions.y;
          i_DispatchDimensions.z = 1;
        } else {
          LOW_ASSERT(false, "Unknown dispatch dimensions type");
        }

        Backend::callbacks().compute_dispatch(get_context().get_context(),
                                              i_DispatchDimensions);

        if (get_context().get_debug_enabled()) {
          LOW_RENDERER_END_RENDERDOC_SECTION(get_context().get_context());
        }
      }

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_execute
    }

  } // namespace Renderer
} // namespace Low
