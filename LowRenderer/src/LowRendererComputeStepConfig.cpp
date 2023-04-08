#include "LowRendererComputeStepConfig.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowRendererComputeStep.h"

namespace Low {
  namespace Renderer {
    const uint16_t ComputeStepConfig::TYPE_ID = 5;
    uint32_t ComputeStepConfig::ms_Capacity = 0u;
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
      uint32_t l_Index = create_instance();

      ComputeStepConfig l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = ComputeStepConfig::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, ComputeStepConfig, callbacks,
                              ComputeStepCallbacks)) ComputeStepCallbacks();
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
      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowRenderer), N(ComputeStepConfig));

      initialize_buffer(&ms_Buffer, ComputeStepConfigData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_ComputeStepConfig);
      LOW_PROFILE_ALLOC(type_slots_ComputeStepConfig);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(ComputeStepConfig);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &ComputeStepConfig::is_alive;
      l_TypeInfo.destroy = &ComputeStepConfig::destroy;
      l_TypeInfo.component = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(callbacks);
        l_PropertyInfo.dataOffset = offsetof(ComputeStepConfigData, callbacks);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ComputeStepConfig,
                                            callbacks, ComputeStepCallbacks);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, ComputeStepConfig, callbacks,
                            ComputeStepCallbacks) =
              *(ComputeStepCallbacks *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
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
      return ms_Capacity;
    }

    ComputeStepCallbacks &ComputeStepConfig::get_callbacks() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(ComputeStepConfig, callbacks, ComputeStepCallbacks);
    }
    void ComputeStepConfig::set_callbacks(ComputeStepCallbacks &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(ComputeStepConfig, callbacks, ComputeStepCallbacks) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_callbacks
      // LOW_CODEGEN::END::CUSTOM:SETTER_callbacks
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

      l_Config.get_callbacks().setup_pipelines = &ComputeStep::create_pipelines;
      l_Config.get_callbacks().setup_signatures =
          &ComputeStep::create_signatures;
      l_Config.get_callbacks().populate_signatures =
          &ComputeStep::prepare_signatures;
      l_Config.get_callbacks().execute = &ComputeStep::default_execute;

      if (p_Node["resources"]) {
        parse_resource_configs(p_Node["resources"], l_Config.get_resources());
      }
      parse_compute_pipeline_configs(p_Node["pipelines"],
                                     l_Config.get_pipelines());

      return l_Config;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint32_t ComputeStepConfig::create_instance()
    {
      uint32_t l_Index = 0u;

      for (; l_Index < get_capacity(); ++l_Index) {
        if (!ms_Slots[l_Index].m_Occupied) {
          break;
        }
      }
      if (l_Index >= get_capacity()) {
        increase_budget();
      }
      ms_Slots[l_Index].m_Occupied = true;
      return l_Index;
    }

    void ComputeStepConfig::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(ComputeStepConfigData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(ComputeStepConfigData, callbacks) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(ComputeStepConfigData, callbacks) *
                          (l_Capacity)],
               l_Capacity * sizeof(ComputeStepCallbacks));
      }
      {
        memcpy(&l_NewBuffer[offsetof(ComputeStepConfigData, resources) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(ComputeStepConfigData, resources) *
                          (l_Capacity)],
               l_Capacity * sizeof(Util::List<ResourceConfig>));
      }
      {
        memcpy(&l_NewBuffer[offsetof(ComputeStepConfigData, pipelines) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(ComputeStepConfigData, pipelines) *
                          (l_Capacity)],
               l_Capacity * sizeof(Util::List<ComputePipelineConfig>));
      }
      {
        memcpy(&l_NewBuffer[offsetof(ComputeStepConfigData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(ComputeStepConfigData, name) * (l_Capacity)],
               l_Capacity * sizeof(Low::Util::Name));
      }
      for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease; ++i) {
        l_NewSlots[i].m_Occupied = false;
        l_NewSlots[i].m_Generation = 0;
      }
      free(ms_Buffer);
      free(ms_Slots);
      ms_Buffer = l_NewBuffer;
      ms_Slots = l_NewSlots;
      ms_Capacity = l_Capacity + l_CapacityIncrease;

      LOW_LOG_DEBUG << "Auto-increased budget for ComputeStepConfig from "
                    << l_Capacity << " to " << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }
  } // namespace Renderer
} // namespace Low
