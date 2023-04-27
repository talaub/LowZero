#include "LowRendererMaterialType.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Renderer {
    const uint16_t MaterialType::TYPE_ID = 16;
    uint32_t MaterialType::ms_Capacity = 0u;
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
      uint32_t l_Index = create_instance();

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
      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowRenderer), N(MaterialType));

      initialize_buffer(&ms_Buffer, MaterialTypeData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_MaterialType);
      LOW_PROFILE_ALLOC(type_slots_MaterialType);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(MaterialType);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &MaterialType::is_alive;
      l_TypeInfo.destroy = &MaterialType::destroy;
      l_TypeInfo.serialize = &MaterialType::serialize;
      l_TypeInfo.deserialize = &MaterialType::deserialize;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &MaterialType::living_instances);
      l_TypeInfo.get_living_count = &MaterialType::living_count;
      l_TypeInfo.component = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(gbuffer_pipeline);
        l_PropertyInfo.editorProperty = false;
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
        l_PropertyInfo.editorProperty = false;
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
        l_PropertyInfo.editorProperty = false;
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
        l_PropertyInfo.editorProperty = false;
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

    MaterialType MaterialType::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      MaterialType l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = MaterialType::TYPE_ID;

      return l_Handle;
    }

    bool MaterialType::is_alive() const
    {
      return m_Data.m_Type == MaterialType::TYPE_ID &&
             check_alive(ms_Slots, MaterialType::get_capacity());
    }

    uint32_t MaterialType::get_capacity()
    {
      return ms_Capacity;
    }

    MaterialType MaterialType::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end();
           ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
    }

    void MaterialType::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void MaterialType::serialize(Low::Util::Handle p_Handle,
                                 Low::Util::Yaml::Node &p_Node)
    {
      MaterialType l_MaterialType = p_Handle.get_id();
      l_MaterialType.serialize(p_Node);
    }

    Low::Util::Handle MaterialType::deserialize(Low::Util::Yaml::Node &p_Node,
                                                Low::Util::Handle p_Creator)
    {
      MaterialType l_Handle = MaterialType::make(N(MaterialType));

      l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
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

    uint32_t MaterialType::create_instance()
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

    void MaterialType::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(MaterialTypeData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(MaterialTypeData, gbuffer_pipeline) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialTypeData, gbuffer_pipeline) *
                          (l_Capacity)],
               l_Capacity * sizeof(GraphicsPipelineConfig));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MaterialTypeData, depth_pipeline) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialTypeData, depth_pipeline) *
                          (l_Capacity)],
               l_Capacity * sizeof(GraphicsPipelineConfig));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr =
              new (&l_NewBuffer[offsetof(MaterialTypeData, properties) *
                                    (l_Capacity + l_CapacityIncrease) +
                                (it->get_index() *
                                 sizeof(Util::List<MaterialTypeProperty>))])
                  Util::List<MaterialTypeProperty>();
          *i_ValPtr = it->get_properties();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(MaterialTypeData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialTypeData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for MaterialType from "
                    << l_Capacity << " to " << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }
  } // namespace Renderer
} // namespace Low
