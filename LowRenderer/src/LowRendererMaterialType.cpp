#include "LowRendererMaterialType.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Renderer {
    const uint16_t MaterialType::TYPE_ID = 10;
    uint8_t *MaterialType::ms_Buffer = 0;
    Low::Util::Instances::Slot *MaterialType::ms_Slots = 0;
    Low::Util::List<MaterialType> MaterialType::ms_LivingInstances =
        Low::Util::List<MaterialType>();

    MaterialType::MaterialType() : Low::Util::Handle(0ull)
    {
    }
    MaterialType::MaterialType(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    MaterialType::MaterialType(MaterialType &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    MaterialType MaterialType::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = Low::Util::Instances::create_instance(
          ms_Buffer, ms_Slots, get_capacity());

      MaterialType l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = MaterialType::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, MaterialType, gbuffer_pipeline,
                              GraphicsPipelineConfig)) GraphicsPipelineConfig();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MaterialType, depth_pipeline,
                              GraphicsPipelineConfig)) GraphicsPipelineConfig();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MaterialType, properties,
                              Util::List<MaterialTypeProperty>))
          Util::List<MaterialTypeProperty>();
      ACCESSOR_TYPE_SOA(l_Handle, MaterialType, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      return l_Handle;
    }

    void MaterialType::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const MaterialType *l_Instances = living_instances();
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

    void MaterialType::initialize()
    {
      initialize_buffer(&ms_Buffer, MaterialTypeData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_MaterialType);
      LOW_PROFILE_ALLOC(type_slots_MaterialType);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(MaterialType);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &MaterialType::is_alive;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(gbuffer_pipeline);
        l_PropertyInfo.dataOffset =
            offsetof(MaterialTypeData, gbuffer_pipeline);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MaterialType, gbuffer_pipeline, GraphicsPipelineConfig);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, MaterialType, gbuffer_pipeline,
                            GraphicsPipelineConfig) =
              *(GraphicsPipelineConfig *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(depth_pipeline);
        l_PropertyInfo.dataOffset = offsetof(MaterialTypeData, depth_pipeline);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MaterialType, depth_pipeline, GraphicsPipelineConfig);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, MaterialType, depth_pipeline,
                            GraphicsPipelineConfig) =
              *(GraphicsPipelineConfig *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(properties);
        l_PropertyInfo.dataOffset = offsetof(MaterialTypeData, properties);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MaterialType, properties,
                                            Util::List<MaterialTypeProperty>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, MaterialType, properties,
                            Util::List<MaterialTypeProperty>) =
              *(Util::List<MaterialTypeProperty> *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.dataOffset = offsetof(MaterialTypeData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MaterialType, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, MaterialType, name, Low::Util::Name) =
              *(Low::Util::Name *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void MaterialType::cleanup()
    {
      Low::Util::List<MaterialType> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_MaterialType);
      LOW_PROFILE_FREE(type_slots_MaterialType);
    }

    bool MaterialType::is_alive() const
    {
      return m_Data.m_Type == MaterialType::TYPE_ID &&
             check_alive(ms_Slots, MaterialType::get_capacity());
    }

    uint32_t MaterialType::get_capacity()
    {
      static uint32_t l_Capacity = 0u;
      if (l_Capacity == 0u) {
        l_Capacity =
            Low::Util::Config::get_capacity(N(LowRenderer), N(MaterialType));
      }
      return l_Capacity;
    }

    GraphicsPipelineConfig &MaterialType::get_gbuffer_pipeline() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(MaterialType, gbuffer_pipeline, GraphicsPipelineConfig);
    }
    void MaterialType::set_gbuffer_pipeline(GraphicsPipelineConfig &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(MaterialType, gbuffer_pipeline, GraphicsPipelineConfig) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gbuffer_pipeline
      // LOW_CODEGEN::END::CUSTOM:SETTER_gbuffer_pipeline
    }

    GraphicsPipelineConfig &MaterialType::get_depth_pipeline() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(MaterialType, depth_pipeline, GraphicsPipelineConfig);
    }
    void MaterialType::set_depth_pipeline(GraphicsPipelineConfig &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(MaterialType, depth_pipeline, GraphicsPipelineConfig) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_depth_pipeline
      // LOW_CODEGEN::END::CUSTOM:SETTER_depth_pipeline
    }

    Util::List<MaterialTypeProperty> &MaterialType::get_properties() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(MaterialType, properties,
                      Util::List<MaterialTypeProperty>);
    }
    void MaterialType::set_properties(Util::List<MaterialTypeProperty> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(MaterialType, properties, Util::List<MaterialTypeProperty>) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_properties
      // LOW_CODEGEN::END::CUSTOM:SETTER_properties
    }

    Low::Util::Name MaterialType::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(MaterialType, name, Low::Util::Name);
    }
    void MaterialType::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(MaterialType, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

  } // namespace Renderer
} // namespace Low
