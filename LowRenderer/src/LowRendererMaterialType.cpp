#include "LowRendererMaterialType.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
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
    uint32_t MaterialType::ms_PageSize = 0u;
    Low::Util::SharedMutex MaterialType::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        MaterialType::ms_PagesLock(MaterialType::ms_PagesMutex,
                                   std::defer_lock);
    Low::Util::List<MaterialType> MaterialType::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        MaterialType::ms_Pages;

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
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      MaterialType l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = MaterialType::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, MaterialType, gbuffer_pipeline,
          GraphicsPipelineConfig)) GraphicsPipelineConfig();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, MaterialType, depth_pipeline,
          GraphicsPipelineConfig)) GraphicsPipelineConfig();
      ACCESSOR_TYPE_SOA(l_Handle, MaterialType, internal, bool) =
          false;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, MaterialType, properties,
                                 Util::List<MaterialTypeProperty>))
          Util::List<MaterialTypeProperty>();
      ACCESSOR_TYPE_SOA(l_Handle, MaterialType, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void MaterialType::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<MaterialType> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      _LOW_ASSERT(
          get_page_for_index(get_index(), l_PageIndex, l_SlotIndex));
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
          l_Page->mutex);
      l_Page->slots[l_SlotIndex].m_Occupied = false;
      l_Page->slots[l_SlotIndex].m_Generation++;

      ms_PagesLock.lock();
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end();) {
        if (it->get_id() == get_id()) {
          it = ms_LivingInstances.erase(it);
        } else {
          it++;
        }
      }
      ms_PagesLock.unlock();
    }

    void MaterialType::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                    N(MaterialType));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, MaterialType::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

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
            offsetof(MaterialType::Data, gbuffer_pipeline);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
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
            offsetof(MaterialType::Data, depth_pipeline);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
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
            offsetof(MaterialType::Data, internal);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
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
            offsetof(MaterialType::Data, properties);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset =
            offsetof(MaterialType::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
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
      ms_PagesLock.lock();
      for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
        Low::Util::Instances::Page *i_Page = *it;
        free(i_Page->buffer);
        free(i_Page->slots);
        free(i_Page->lockWords);
        delete i_Page;
        it = ms_Pages.erase(it);
      }

      ms_Capacity = 0;

      ms_PagesLock.unlock();
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
      l_Handle.m_Data.m_Type = MaterialType::TYPE_ID;

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
        l_Handle.m_Data.m_Generation = 0;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
          l_Page->mutex);
      l_Handle.m_Data.m_Generation =
          l_Page->slots[l_SlotIndex].m_Generation;

      return l_Handle;
    }

    MaterialType MaterialType::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      MaterialType l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = MaterialType::TYPE_ID;

      return l_Handle;
    }

    bool MaterialType::is_alive() const
    {
      if (m_Data.m_Type != MaterialType::TYPE_ID) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
          l_Page->mutex);
      return m_Data.m_Type == MaterialType::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
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

      // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME
      // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return Low::Util::Handle::DEAD;
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

    u64 MaterialType::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
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
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gbuffer_pipeline

      // LOW_CODEGEN::END::CUSTOM:GETTER_gbuffer_pipeline

      return TYPE_SOA(MaterialType, gbuffer_pipeline,
                      GraphicsPipelineConfig);
    }
    void MaterialType::set_gbuffer_pipeline(
        GraphicsPipelineConfig &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gbuffer_pipeline

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gbuffer_pipeline

      // Set new value
      TYPE_SOA(MaterialType, gbuffer_pipeline,
               GraphicsPipelineConfig) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gbuffer_pipeline

      // LOW_CODEGEN::END::CUSTOM:SETTER_gbuffer_pipeline

      broadcast_observable(N(gbuffer_pipeline));
    }

    GraphicsPipelineConfig &MaterialType::get_depth_pipeline() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_depth_pipeline

      // LOW_CODEGEN::END::CUSTOM:GETTER_depth_pipeline

      return TYPE_SOA(MaterialType, depth_pipeline,
                      GraphicsPipelineConfig);
    }
    void
    MaterialType::set_depth_pipeline(GraphicsPipelineConfig &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_depth_pipeline

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_depth_pipeline

      // Set new value
      TYPE_SOA(MaterialType, depth_pipeline, GraphicsPipelineConfig) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_depth_pipeline

      // LOW_CODEGEN::END::CUSTOM:SETTER_depth_pipeline

      broadcast_observable(N(depth_pipeline));
    }

    bool MaterialType::is_internal() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_internal

      // LOW_CODEGEN::END::CUSTOM:GETTER_internal

      return TYPE_SOA(MaterialType, internal, bool);
    }
    void MaterialType::toggle_internal()
    {
      set_internal(!is_internal());
    }

    void MaterialType::set_internal(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_internal

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_internal

      // Set new value
      TYPE_SOA(MaterialType, internal, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_internal

      // LOW_CODEGEN::END::CUSTOM:SETTER_internal

      broadcast_observable(N(internal));
    }

    Util::List<MaterialTypeProperty> &
    MaterialType::get_properties() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_properties

      // LOW_CODEGEN::END::CUSTOM:GETTER_properties

      return TYPE_SOA(MaterialType, properties,
                      Util::List<MaterialTypeProperty>);
    }
    void MaterialType::set_properties(
        Util::List<MaterialTypeProperty> &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_properties

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_properties

      // Set new value
      TYPE_SOA(MaterialType, properties,
               Util::List<MaterialTypeProperty>) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_properties

      // LOW_CODEGEN::END::CUSTOM:SETTER_properties

      broadcast_observable(N(properties));
    }

    Low::Util::Name MaterialType::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(MaterialType, name, Low::Util::Name);
    }
    void MaterialType::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(MaterialType, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t MaterialType::create_instance(
        u32 &p_PageIndex, u32 &p_SlotIndex,
        Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock)
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      u32 l_Index = 0;
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      bool l_FoundIndex = false;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;

      for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
           ++l_PageIndex) {
        Low::Util::UniqueLock<Low::Util::Mutex> i_PageLock(
            ms_Pages[l_PageIndex]->mutex);
        for (l_SlotIndex = 0;
             l_SlotIndex < ms_Pages[l_PageIndex]->size;
             ++l_SlotIndex) {
          if (!ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied) {
            l_FoundIndex = true;
            l_PageLock = std::move(i_PageLock);
            break;
          }
          l_Index++;
        }
        if (l_FoundIndex) {
          break;
        }
      }
      if (!l_FoundIndex) {
        l_SlotIndex = 0;
        l_PageIndex = create_page();
        Low::Util::UniqueLock<Low::Util::Mutex> l_NewLock(
            ms_Pages[l_PageIndex]->mutex);
        l_PageLock = std::move(l_NewLock);
      }
      ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
      p_PageIndex = l_PageIndex;
      p_SlotIndex = l_SlotIndex;
      p_PageLock = std::move(l_PageLock);
      LOCK_UNLOCK(l_PagesLock);
      return l_Index;
    }

    u32 MaterialType::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for MaterialType.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, MaterialType::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool MaterialType::get_page_for_index(const u32 p_Index,
                                          u32 &p_PageIndex,
                                          u32 &p_SlotIndex)
    {
      if (p_Index >= get_capacity()) {
        p_PageIndex = LOW_UINT32_MAX;
        p_SlotIndex = LOW_UINT32_MAX;
        return false;
      }
      p_PageIndex = p_Index / ms_PageSize;
      if (p_PageIndex > (ms_Pages.size() - 1)) {
        return false;
      }
      p_SlotIndex = p_Index - (ms_PageSize * p_PageIndex);
      return true;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
