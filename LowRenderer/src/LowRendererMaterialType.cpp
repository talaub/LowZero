#include "LowRendererMaterialType.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t MaterialType::TYPE_ID = 16;
    uint32_t MaterialType::ms_Capacity = 0u;
    uint8_t *MaterialType::ms_Buffer = 0;
    std::shared_mutex MaterialType::ms_BufferMutex;
    Low::Util::Instances::Slot *MaterialType::ms_Slots = 0;
    Low::Util::List<MaterialType> MaterialType::ms_LivingInstances =
        Low::Util::List<MaterialType>();

    MaterialType::MaterialType() : Low::Util::Handle(0ull)
    {
    }
    MaterialType::MaterialType(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    MaterialType::MaterialType(MaterialType &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle MaterialType::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    MaterialType MaterialType::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      MaterialType l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = MaterialType::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(
          l_Handle, MaterialType, gbuffer_pipeline,
          GraphicsPipelineConfig)) GraphicsPipelineConfig();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MaterialType, depth_pipeline,
                              GraphicsPipelineConfig))
          GraphicsPipelineConfig();
      ACCESSOR_TYPE_SOA(l_Handle, MaterialType, internal, bool) =
          false;
      new (&ACCESSOR_TYPE_SOA(l_Handle, MaterialType, properties,
                              Util::List<MaterialTypeProperty>))
          Util::List<MaterialTypeProperty>();
      ACCESSOR_TYPE_SOA(l_Handle, MaterialType, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void MaterialType::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

      // LOW_CODEGEN::END::CUSTOM:DESTROY

      broadcast_observable(OBSERVABLE_DESTROY);

      WRITE_LOCK(l_Lock);
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
    }

    void MaterialType::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                    N(MaterialType));

      initialize_buffer(&ms_Buffer, MaterialTypeData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_MaterialType);
      LOW_PROFILE_ALLOC(type_slots_MaterialType);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(MaterialType);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &MaterialType::is_alive;
      l_TypeInfo.destroy = &MaterialType::destroy;
      l_TypeInfo.serialize = &MaterialType::serialize;
      l_TypeInfo.deserialize = &MaterialType::deserialize;
      l_TypeInfo.find_by_index = &MaterialType::_find_by_index;
      l_TypeInfo.notify = &MaterialType::_notify;
      l_TypeInfo.find_by_name = &MaterialType::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &MaterialType::_make;
      l_TypeInfo.duplicate_default = &MaterialType::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &MaterialType::living_instances);
      l_TypeInfo.get_living_count = &MaterialType::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: gbuffer_pipeline
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(gbuffer_pipeline);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialTypeData, gbuffer_pipeline);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.get_gbuffer_pipeline();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MaterialType,
                                            gbuffer_pipeline,
                                            GraphicsPipelineConfig);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.set_gbuffer_pipeline(
              *(GraphicsPipelineConfig *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MaterialType l_Handle = p_Handle.get_id();
          *((GraphicsPipelineConfig *)p_Data) =
              l_Handle.get_gbuffer_pipeline();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: gbuffer_pipeline
      }
      {
        // Property: depth_pipeline
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(depth_pipeline);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialTypeData, depth_pipeline);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.get_depth_pipeline();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MaterialType,
                                            depth_pipeline,
                                            GraphicsPipelineConfig);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.set_depth_pipeline(
              *(GraphicsPipelineConfig *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MaterialType l_Handle = p_Handle.get_id();
          *((GraphicsPipelineConfig *)p_Data) =
              l_Handle.get_depth_pipeline();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: depth_pipeline
      }
      {
        // Property: internal
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(internal);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialTypeData, internal);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.is_internal();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MaterialType,
                                            internal, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.set_internal(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MaterialType l_Handle = p_Handle.get_id();
          *((bool *)p_Data) = l_Handle.is_internal();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: internal
      }
      {
        // Property: properties
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(properties);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialTypeData, properties);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.get_properties();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MaterialType, properties,
              Util::List<MaterialTypeProperty>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.set_properties(
              *(Util::List<MaterialTypeProperty> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MaterialType l_Handle = p_Handle.get_id();
          *((Util::List<MaterialTypeProperty> *)p_Data) =
              l_Handle.get_properties();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: properties
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MaterialTypeData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MaterialType,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MaterialType l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void MaterialType::cleanup()
    {
      Low::Util::List<MaterialType> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_MaterialType);
      LOW_PROFILE_FREE(type_slots_MaterialType);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle MaterialType::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
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
      READ_LOCK(l_Lock);
      return m_Data.m_Type == MaterialType::TYPE_ID &&
             check_alive(ms_Slots, MaterialType::get_capacity());
    }

    uint32_t MaterialType::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    MaterialType::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    MaterialType MaterialType::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    MaterialType MaterialType::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      MaterialType l_Handle = make(p_Name);
      l_Handle.set_gbuffer_pipeline(get_gbuffer_pipeline());
      l_Handle.set_depth_pipeline(get_depth_pipeline());
      l_Handle.set_internal(is_internal());
      l_Handle.set_properties(get_properties());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    MaterialType MaterialType::duplicate(MaterialType p_Handle,
                                         Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    MaterialType::_duplicate(Low::Util::Handle p_Handle,
                             Low::Util::Name p_Name)
    {
      MaterialType l_MaterialType = p_Handle.get_id();
      return l_MaterialType.duplicate(p_Name);
    }

    void MaterialType::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      p_Node = get_name().c_str();
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void MaterialType::serialize(Low::Util::Handle p_Handle,
                                 Low::Util::Yaml::Node &p_Node)
    {
      MaterialType l_MaterialType = p_Handle.get_id();
      l_MaterialType.serialize(p_Node);
    }

    Low::Util::Handle
    MaterialType::deserialize(Low::Util::Yaml::Node &p_Node,
                              Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      return MaterialType::find_by_name(LOW_YAML_AS_NAME(p_Node))
          .get_id();
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void MaterialType::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 MaterialType::observe(Low::Util::Name p_Observable,
                              Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void MaterialType::notify(Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void MaterialType::_notify(Low::Util::Handle p_Observer,
                               Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
    {
      MaterialType l_MaterialType = p_Observer.get_id();
      l_MaterialType.notify(p_Observed, p_Observable);
    }

    GraphicsPipelineConfig &MaterialType::get_gbuffer_pipeline() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gbuffer_pipeline

      // LOW_CODEGEN::END::CUSTOM:GETTER_gbuffer_pipeline

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MaterialType, gbuffer_pipeline,
                      GraphicsPipelineConfig);
    }
    void MaterialType::set_gbuffer_pipeline(
        GraphicsPipelineConfig &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gbuffer_pipeline

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gbuffer_pipeline

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MaterialType, gbuffer_pipeline,
               GraphicsPipelineConfig) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gbuffer_pipeline

      // LOW_CODEGEN::END::CUSTOM:SETTER_gbuffer_pipeline

      broadcast_observable(N(gbuffer_pipeline));
    }

    GraphicsPipelineConfig &MaterialType::get_depth_pipeline() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_depth_pipeline

      // LOW_CODEGEN::END::CUSTOM:GETTER_depth_pipeline

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MaterialType, depth_pipeline,
                      GraphicsPipelineConfig);
    }
    void
    MaterialType::set_depth_pipeline(GraphicsPipelineConfig &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_depth_pipeline

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_depth_pipeline

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MaterialType, depth_pipeline, GraphicsPipelineConfig) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_depth_pipeline

      // LOW_CODEGEN::END::CUSTOM:SETTER_depth_pipeline

      broadcast_observable(N(depth_pipeline));
    }

    bool MaterialType::is_internal() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_internal

      // LOW_CODEGEN::END::CUSTOM:GETTER_internal

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MaterialType, internal, bool);
    }
    void MaterialType::toggle_internal()
    {
      set_internal(!is_internal());
    }

    void MaterialType::set_internal(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_internal

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_internal

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MaterialType, internal, bool) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_internal

      // LOW_CODEGEN::END::CUSTOM:SETTER_internal

      broadcast_observable(N(internal));
    }

    Util::List<MaterialTypeProperty> &
    MaterialType::get_properties() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_properties

      // LOW_CODEGEN::END::CUSTOM:GETTER_properties

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MaterialType, properties,
                      Util::List<MaterialTypeProperty>);
    }
    void MaterialType::set_properties(
        Util::List<MaterialTypeProperty> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_properties

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_properties

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MaterialType, properties,
               Util::List<MaterialTypeProperty>) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_properties

      // LOW_CODEGEN::END::CUSTOM:SETTER_properties

      broadcast_observable(N(properties));
    }

    Low::Util::Name MaterialType::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MaterialType, name, Low::Util::Name);
    }
    void MaterialType::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MaterialType, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
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
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer =
          (uint8_t *)malloc((l_Capacity + l_CapacityIncrease) *
                            sizeof(MaterialTypeData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(
            &l_NewBuffer[offsetof(MaterialTypeData,
                                  gbuffer_pipeline) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MaterialTypeData, gbuffer_pipeline) *
                       (l_Capacity)],
            l_Capacity * sizeof(GraphicsPipelineConfig));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(MaterialTypeData, depth_pipeline) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MaterialTypeData, depth_pipeline) *
                       (l_Capacity)],
            l_Capacity * sizeof(GraphicsPipelineConfig));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MaterialTypeData, internal) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialTypeData, internal) *
                          (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          MaterialType i_MaterialType = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(MaterialTypeData, properties) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(
                                Util::List<MaterialTypeProperty>))])
              Util::List<MaterialTypeProperty>();
          *i_ValPtr = ACCESSOR_TYPE_SOA(
              i_MaterialType, MaterialType, properties,
              Util::List<MaterialTypeProperty>);
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(MaterialTypeData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialTypeData, name) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Util::Name));
      }
      for (uint32_t i = l_Capacity;
           i < l_Capacity + l_CapacityIncrease; ++i) {
        l_NewSlots[i].m_Occupied = false;
        l_NewSlots[i].m_Generation = 0;
      }
      free(ms_Buffer);
      free(ms_Slots);
      ms_Buffer = l_NewBuffer;
      ms_Slots = l_NewSlots;
      ms_Capacity = l_Capacity + l_CapacityIncrease;

      LOW_LOG_DEBUG << "Auto-increased budget for MaterialType from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
