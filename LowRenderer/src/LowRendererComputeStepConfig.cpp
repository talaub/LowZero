#include "LowRendererComputeStepConfig.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Renderer {
    const uint16_t ComputeStepConfig::TYPE_ID = 3;
    uint8_t *ComputeStepConfig::ms_Buffer = 0;
    Low::Util::Instances::Slot *ComputeStepConfig::ms_Slots = 0;
    Low::Util::List<ComputeStepConfig> ComputeStepConfig::ms_LivingInstances =
        Low::Util::List<ComputeStepConfig>();

    ComputeStepConfig::ComputeStepConfig() : Low::Util::Handle(0ull)
    {
    }
    ComputeStepConfig::ComputeStepConfig(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    ComputeStepConfig::ComputeStepConfig(ComputeStepConfig &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    ComputeStepConfig ComputeStepConfig::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = Low::Util::Instances::create_instance(
          ms_Buffer, ms_Slots, get_capacity());

      ComputeStepConfig l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = ComputeStepConfig::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, ComputeStepConfig, resources,
                              Util::List<ResourceConfig>))
          Util::List<ResourceConfig>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, ComputeStepConfig, pipelines,
                              Util::List<ComputePipelineConfig>))
          Util::List<ComputePipelineConfig>();
      ACCESSOR_TYPE_SOA(l_Handle, ComputeStepConfig, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      return l_Handle;
    }

    void ComputeStepConfig::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const ComputeStepConfig *l_Instances = living_instances();
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

    void ComputeStepConfig::initialize()
    {
      initialize_buffer(&ms_Buffer, ComputeStepConfigData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_ComputeStepConfig);
      LOW_PROFILE_ALLOC(type_slots_ComputeStepConfig);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(ComputeStepConfig);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &ComputeStepConfig::is_alive;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resources);
        l_PropertyInfo.dataOffset = offsetof(ComputeStepConfigData, resources);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ComputeStepConfig,
                                            resources,
                                            Util::List<ResourceConfig>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, ComputeStepConfig, resources,
                            Util::List<ResourceConfig>) =
              *(Util::List<ResourceConfig> *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(pipelines);
        l_PropertyInfo.dataOffset = offsetof(ComputeStepConfigData, pipelines);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ComputeStepConfig,
                                            pipelines,
                                            Util::List<ComputePipelineConfig>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, ComputeStepConfig, pipelines,
                            Util::List<ComputePipelineConfig>) =
              *(Util::List<ComputePipelineConfig> *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.dataOffset = offsetof(ComputeStepConfigData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ComputeStepConfig, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, ComputeStepConfig, name,
                            Low::Util::Name) = *(Low::Util::Name *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void ComputeStepConfig::cleanup()
    {
      Low::Util::List<ComputeStepConfig> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_ComputeStepConfig);
      LOW_PROFILE_FREE(type_slots_ComputeStepConfig);
    }

    bool ComputeStepConfig::is_alive() const
    {
      return m_Data.m_Type == ComputeStepConfig::TYPE_ID &&
             check_alive(ms_Slots, ComputeStepConfig::get_capacity());
    }

    uint32_t ComputeStepConfig::get_capacity()
    {
      static uint32_t l_Capacity = 0u;
      if (l_Capacity == 0u) {
        l_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                     N(ComputeStepConfig));
      }
      return l_Capacity;
    }

    Util::List<ResourceConfig> &ComputeStepConfig::get_resources() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(ComputeStepConfig, resources, Util::List<ResourceConfig>);
    }

    Util::List<ComputePipelineConfig> &ComputeStepConfig::get_pipelines() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(ComputeStepConfig, pipelines,
                      Util::List<ComputePipelineConfig>);
    }

    Low::Util::Name ComputeStepConfig::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(ComputeStepConfig, name, Low::Util::Name);
    }
    void ComputeStepConfig::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(ComputeStepConfig, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    ComputeStepConfig ComputeStepConfig::make(Util::Name p_Name,
                                              Util::Yaml::Node &p_Node)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      ComputeStepConfig l_Config = ComputeStepConfig::make(p_Name);

      if (p_Node["resources"]) {
        parse_resource_configs(p_Node["resources"], l_Config.get_resources());
      }
      parse_compute_pipeline_configs(p_Node["pipelines"],
                                     l_Config.get_pipelines());

      return l_Config;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

  } // namespace Renderer
} // namespace Low
