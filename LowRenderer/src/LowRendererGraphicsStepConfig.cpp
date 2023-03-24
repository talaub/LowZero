#include "LowRendererGraphicsStepConfig.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowUtilString.h"

namespace Low {
  namespace Renderer {
    const uint16_t GraphicsStepConfig::TYPE_ID = 4;
    uint8_t *GraphicsStepConfig::ms_Buffer = 0;
    Low::Util::Instances::Slot *GraphicsStepConfig::ms_Slots = 0;
    Low::Util::List<GraphicsStepConfig> GraphicsStepConfig::ms_LivingInstances =
        Low::Util::List<GraphicsStepConfig>();

    GraphicsStepConfig::GraphicsStepConfig() : Low::Util::Handle(0ull)
    {
    }
    GraphicsStepConfig::GraphicsStepConfig(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    GraphicsStepConfig::GraphicsStepConfig(GraphicsStepConfig &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    GraphicsStepConfig GraphicsStepConfig::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = Low::Util::Instances::create_instance(
          ms_Buffer, ms_Slots, get_capacity());

      GraphicsStepConfigData *l_DataPtr =
          (GraphicsStepConfigData
               *)&ms_Buffer[l_Index * sizeof(GraphicsStepConfigData)];
      new (l_DataPtr) GraphicsStepConfigData();

      GraphicsStepConfig l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = GraphicsStepConfig::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, GraphicsStepConfig, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      return l_Handle;
    }

    void GraphicsStepConfig::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const GraphicsStepConfig *l_Instances = living_instances();
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

    void GraphicsStepConfig::initialize()
    {
      initialize_buffer(&ms_Buffer, GraphicsStepConfigData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_GraphicsStepConfig);
      LOW_PROFILE_ALLOC(type_slots_GraphicsStepConfig);
    }

    void GraphicsStepConfig::cleanup()
    {
      Low::Util::List<GraphicsStepConfig> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_GraphicsStepConfig);
      LOW_PROFILE_FREE(type_slots_GraphicsStepConfig);
    }

    bool GraphicsStepConfig::is_alive() const
    {
      return m_Data.m_Type == GraphicsStepConfig::TYPE_ID &&
             check_alive(ms_Slots, GraphicsStepConfig::get_capacity());
    }

    uint32_t GraphicsStepConfig::get_capacity()
    {
      static uint32_t l_Capacity = 0u;
      if (l_Capacity == 0u) {
        l_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                     N(GraphicsStepConfig));
      }
      return l_Capacity;
    }

    Util::List<ResourceConfig> &GraphicsStepConfig::get_resources() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(GraphicsStepConfig, resources,
                      Util::List<ResourceConfig>);
    }

    Util::List<GraphicsPipelineConfig> &
    GraphicsStepConfig::get_pipelines() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(GraphicsStepConfig, pipelines,
                      Util::List<GraphicsPipelineConfig>);
    }

    Util::List<PipelineResourceBindingConfig> &
    GraphicsStepConfig::get_rendertargets() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(GraphicsStepConfig, rendertargets,
                      Util::List<PipelineResourceBindingConfig>);
    }

    Low::Util::Name GraphicsStepConfig::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(GraphicsStepConfig, name, Low::Util::Name);
    }
    void GraphicsStepConfig::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(GraphicsStepConfig, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    GraphicsStepConfig GraphicsStepConfig::make(Util::Name p_Name,
                                                Util::Yaml::Node &p_Node)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      GraphicsStepConfig l_Config = GraphicsStepConfig::make(p_Name);

      if (p_Node["resources"]) {
        parse_resource_configs(p_Node["resources"], l_Config.get_resources());
      }
      Util::Yaml::Node &l_PipelinesNode = p_Node["pipelines"];
      for (auto it = l_PipelinesNode.begin(); it != l_PipelinesNode.end();
           ++it) {
        Util::Name i_PipelineName = LOW_YAML_AS_NAME((*it)["name"]);
        l_Config.get_pipelines().push_back(
            get_graphics_pipeline_config(i_PipelineName));
      }

      Util::String l_ContextPrefix = "context:";
      Util::String l_RenderFlowPrefix = "renderflow:";

      Util::Yaml::Node &l_RenderTargetsNode = p_Node["rendertargets"];
      for (auto it = l_RenderTargetsNode.begin();
           it != l_RenderTargetsNode.end(); ++it) {
        Util::String i_TargetString = LOW_YAML_AS_STRING((*it));
        PipelineResourceBindingConfig i_BindConfig;
        i_BindConfig.resourceScope = ResourceBindScope::LOCAL;
        Util::String i_Name = i_TargetString;

        if (Util::StringHelper::begins_with(i_TargetString, l_ContextPrefix)) {
          i_Name = i_TargetString.substr(l_ContextPrefix.length());
          i_BindConfig.resourceScope = ResourceBindScope::CONTEXT;
        } else if (Util::StringHelper::begins_with(i_TargetString,
                                                   l_RenderFlowPrefix)) {
          i_Name = i_TargetString.substr(l_RenderFlowPrefix.length());
          i_BindConfig.resourceScope = ResourceBindScope::RENDERFLOW;
        }
        i_BindConfig.resourceName = LOW_NAME(i_Name.c_str());

        l_Config.get_rendertargets().push_back(i_BindConfig);
      }

      return l_Config;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

  } // namespace Renderer
} // namespace Low
