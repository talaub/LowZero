#include "LowCoreRegion.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowCoreEntity.h"
#include "LowUtilFileIO.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Region::TYPE_ID = 19;
    uint32_t Region::ms_Capacity = 0u;
    uint32_t Region::ms_PageSize = 0u;
    Low::Util::SharedMutex Region::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        Region::ms_PagesLock(Region::ms_PagesMutex, std::defer_lock);
    Low::Util::List<Region> Region::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *> Region::ms_Pages;

    Region::Region() : Low::Util::Handle(0ull)
    {
    }
    Region::Region(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Region::Region(Region &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Region::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Region Region::make(Low::Util::Name p_Name)
    {
      return make(p_Name, 0ull);
    }

    Region Region::make(Low::Util::Name p_Name,
                        Low::Util::UniqueId p_UniqueId)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      Region l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = Region::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<Region> l_HandleLock(l_Handle);

      ACCESSOR_TYPE_SOA(l_Handle, Region, loaded, bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, Region, streaming_enabled, bool) =
          false;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Region, streaming_position,
                                 Math::Vector3)) Math::Vector3();
      ACCESSOR_TYPE_SOA(l_Handle, Region, streaming_radius, float) =
          0.0f;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Region, entities,
                                 Util::Set<Util::UniqueId>))
          Util::Set<Util::UniqueId>();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Region, scene, Scene))
          Scene();
      ACCESSOR_TYPE_SOA(l_Handle, Region, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      if (p_UniqueId > 0ull) {
        l_Handle.set_unique_id(p_UniqueId);
      } else {
        l_Handle.set_unique_id(
            Low::Util::generate_unique_id(l_Handle.get_id()));
      }
      Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                    l_Handle.get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Region::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<Region> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      Low::Util::remove_unique_id(get_unique_id());

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

    void Region::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowCore), N(Region));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, Region::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Region);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Region::is_alive;
      l_TypeInfo.destroy = &Region::destroy;
      l_TypeInfo.serialize = &Region::serialize;
      l_TypeInfo.deserialize = &Region::deserialize;
      l_TypeInfo.find_by_index = &Region::_find_by_index;
      l_TypeInfo.notify = &Region::_notify;
      l_TypeInfo.find_by_name = &Region::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Region::_make;
      l_TypeInfo.duplicate_default = &Region::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Region::living_instances);
      l_TypeInfo.get_living_count = &Region::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: loaded
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(loaded);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Region::Data, loaded);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Region l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Region> l_HandleLock(l_Handle);
          l_Handle.is_loaded();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Region, loaded,
                                            bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Region l_Handle = p_Handle.get_id();
          l_Handle.set_loaded(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Region l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Region> l_HandleLock(l_Handle);
          *((bool *)p_Data) = l_Handle.is_loaded();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: loaded
      }
      {
        // Property: streaming_enabled
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(streaming_enabled);
        l_PropertyInfo.editorProperty = true;
        l_PropertyInfo.dataOffset =
            offsetof(Region::Data, streaming_enabled);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Region l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Region> l_HandleLock(l_Handle);
          l_Handle.is_streaming_enabled();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Region,
                                            streaming_enabled, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Region l_Handle = p_Handle.get_id();
          l_Handle.set_streaming_enabled(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Region l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Region> l_HandleLock(l_Handle);
          *((bool *)p_Data) = l_Handle.is_streaming_enabled();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: streaming_enabled
      }
      {
        // Property: streaming_position
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(streaming_position);
        l_PropertyInfo.editorProperty = true;
        l_PropertyInfo.dataOffset =
            offsetof(Region::Data, streaming_position);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Region l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Region> l_HandleLock(l_Handle);
          l_Handle.get_streaming_position();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Region, streaming_position, Math::Vector3);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Region l_Handle = p_Handle.get_id();
          l_Handle.set_streaming_position(*(Math::Vector3 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Region l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Region> l_HandleLock(l_Handle);
          *((Math::Vector3 *)p_Data) =
              l_Handle.get_streaming_position();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: streaming_position
      }
      {
        // Property: streaming_radius
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(streaming_radius);
        l_PropertyInfo.editorProperty = true;
        l_PropertyInfo.dataOffset =
            offsetof(Region::Data, streaming_radius);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Region l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Region> l_HandleLock(l_Handle);
          l_Handle.get_streaming_radius();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Region,
                                            streaming_radius, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Region l_Handle = p_Handle.get_id();
          l_Handle.set_streaming_radius(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Region l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Region> l_HandleLock(l_Handle);
          *((float *)p_Data) = l_Handle.get_streaming_radius();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: streaming_radius
      }
      {
        // Property: entities
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(entities);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Region::Data, entities);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: entities
      }
      {
        // Property: scene
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(scene);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Region::Data, scene);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Scene::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Region l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Region> l_HandleLock(l_Handle);
          l_Handle.get_scene();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Region, scene,
                                            Scene);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Region l_Handle = p_Handle.get_id();
          l_Handle.set_scene(*(Scene *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Region l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Region> l_HandleLock(l_Handle);
          *((Scene *)p_Data) = l_Handle.get_scene();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: scene
      }
      {
        // Property: unique_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(unique_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Region::Data, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Region l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Region> l_HandleLock(l_Handle);
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Region, unique_id, Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Region l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Region> l_HandleLock(l_Handle);
          *((Low::Util::UniqueId *)p_Data) = l_Handle.get_unique_id();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: unique_id
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = true;
        l_PropertyInfo.dataOffset = offsetof(Region::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Region l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Region> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Region, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Region l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Region l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Region> l_HandleLock(l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: serialize_entities
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(serialize_entities);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Node);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: serialize_entities
      }
      {
        // Function: add_entity
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(add_entity);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Entity);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = Entity::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: add_entity
      }
      {
        // Function: remove_entity
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(remove_entity);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Entity);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = Entity::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: remove_entity
      }
      {
        // Function: load_entities
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(load_entities);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: load_entities
      }
      {
        // Function: unload_entities
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(unload_entities);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: unload_entities
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Region::cleanup()
    {
      Low::Util::List<Region> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle Region::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    Region Region::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Region l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = Region::TYPE_ID;

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

    Region Region::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      Region l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = Region::TYPE_ID;

      return l_Handle;
    }

    bool Region::is_alive() const
    {
      if (m_Data.m_Type != Region::TYPE_ID) {
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
      return m_Data.m_Type == Region::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t Region::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle Region::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    Region Region::find_by_name(Low::Util::Name p_Name)
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

    Region Region::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      Region l_Handle = make(p_Name);
      l_Handle.set_loaded(is_loaded());
      l_Handle.set_streaming_enabled(is_streaming_enabled());
      l_Handle.set_streaming_position(get_streaming_position());
      l_Handle.set_streaming_radius(get_streaming_radius());
      if (get_scene().is_alive()) {
        l_Handle.set_scene(get_scene());
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    Region Region::duplicate(Region p_Handle, Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle Region::_duplicate(Low::Util::Handle p_Handle,
                                         Low::Util::Name p_Name)
    {
      Region l_Region = p_Handle.get_id();
      return l_Region.duplicate(p_Name);
    }

    void Region::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["streaming_enabled"] = is_streaming_enabled();
      Low::Util::Serialization::serialize(
          p_Node["streaming_position"], get_streaming_position());
      p_Node["streaming_radius"] = get_streaming_radius();
      p_Node["_unique_id"] =
          Low::Util::hash_to_string(get_unique_id()).c_str();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Region::serialize(Low::Util::Handle p_Handle,
                           Low::Util::Yaml::Node &p_Node)
    {
      Region l_Region = p_Handle.get_id();
      l_Region.serialize(p_Node);
    }

    Low::Util::Handle
    Region::deserialize(Low::Util::Yaml::Node &p_Node,
                        Low::Util::Handle p_Creator)
    {
      Low::Util::UniqueId l_HandleUniqueId = 0ull;
      if (p_Node["unique_id"]) {
        l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
      } else if (p_Node["_unique_id"]) {
        l_HandleUniqueId = Low::Util::string_to_hash(
            LOW_YAML_AS_STRING(p_Node["_unique_id"]));
      }

      Region l_Handle = Region::make(N(Region), l_HandleUniqueId);

      if (p_Node["streaming_enabled"]) {
        l_Handle.set_streaming_enabled(
            p_Node["streaming_enabled"].as<bool>());
      }
      if (p_Node["streaming_position"]) {
        l_Handle.set_streaming_position(
            Low::Util::Serialization::deserialize_vector3(
                p_Node["streaming_position"]));
      }
      if (p_Node["streaming_radius"]) {
        l_Handle.set_streaming_radius(
            p_Node["streaming_radius"].as<float>());
      }
      if (p_Node["unique_id"]) {
        l_Handle.set_unique_id(
            p_Node["unique_id"].as<Low::Util::UniqueId>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void
    Region::broadcast_observable(Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 Region::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 Region::observe(Low::Util::Name p_Observable,
                        Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void Region::notify(Low::Util::Handle p_Observed,
                        Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void Region::_notify(Low::Util::Handle p_Observer,
                         Low::Util::Handle p_Observed,
                         Low::Util::Name p_Observable)
    {
      Region l_Region = p_Observer.get_id();
      l_Region.notify(p_Observed, p_Observable);
    }

    bool Region::is_loaded() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_loaded

      // LOW_CODEGEN::END::CUSTOM:GETTER_loaded

      return TYPE_SOA(Region, loaded, bool);
    }
    void Region::toggle_loaded()
    {
      set_loaded(!is_loaded());
    }

    void Region::set_loaded(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_loaded

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_loaded

      // Set new value
      TYPE_SOA(Region, loaded, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_loaded

      // LOW_CODEGEN::END::CUSTOM:SETTER_loaded

      broadcast_observable(N(loaded));
    }

    bool Region::is_streaming_enabled() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_streaming_enabled

      // LOW_CODEGEN::END::CUSTOM:GETTER_streaming_enabled

      return TYPE_SOA(Region, streaming_enabled, bool);
    }
    void Region::toggle_streaming_enabled()
    {
      set_streaming_enabled(!is_streaming_enabled());
    }

    void Region::set_streaming_enabled(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_streaming_enabled

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_streaming_enabled

      // Set new value
      TYPE_SOA(Region, streaming_enabled, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_streaming_enabled

      // LOW_CODEGEN::END::CUSTOM:SETTER_streaming_enabled

      broadcast_observable(N(streaming_enabled));
    }

    Math::Vector3 &Region::get_streaming_position() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_streaming_position

      // LOW_CODEGEN::END::CUSTOM:GETTER_streaming_position

      return TYPE_SOA(Region, streaming_position, Math::Vector3);
    }
    void Region::set_streaming_position(float p_X, float p_Y,
                                        float p_Z)
    {
      Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
      set_streaming_position(p_Val);
    }

    void Region::set_streaming_position_x(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_streaming_position();
      l_Value.x = p_Value;
      set_streaming_position(l_Value);
    }

    void Region::set_streaming_position_y(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_streaming_position();
      l_Value.y = p_Value;
      set_streaming_position(l_Value);
    }

    void Region::set_streaming_position_z(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_streaming_position();
      l_Value.z = p_Value;
      set_streaming_position(l_Value);
    }

    void Region::set_streaming_position(Math::Vector3 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_streaming_position

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_streaming_position

      // Set new value
      TYPE_SOA(Region, streaming_position, Math::Vector3) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_streaming_position

      // LOW_CODEGEN::END::CUSTOM:SETTER_streaming_position

      broadcast_observable(N(streaming_position));
    }

    float Region::get_streaming_radius() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_streaming_radius

      // LOW_CODEGEN::END::CUSTOM:GETTER_streaming_radius

      return TYPE_SOA(Region, streaming_radius, float);
    }
    void Region::set_streaming_radius(float p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_streaming_radius

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_streaming_radius

      // Set new value
      TYPE_SOA(Region, streaming_radius, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_streaming_radius

      // LOW_CODEGEN::END::CUSTOM:SETTER_streaming_radius

      broadcast_observable(N(streaming_radius));
    }

    Util::Set<Util::UniqueId> &Region::get_entities() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entities

      // LOW_CODEGEN::END::CUSTOM:GETTER_entities

      return TYPE_SOA(Region, entities, Util::Set<Util::UniqueId>);
    }

    Scene Region::get_scene() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_scene

      // LOW_CODEGEN::END::CUSTOM:GETTER_scene

      return TYPE_SOA(Region, scene, Scene);
    }
    void Region::set_scene(Scene p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_scene

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_scene

      // Set new value
      TYPE_SOA(Region, scene, Scene) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_scene

      p_Value.get_regions().insert(get_unique_id());
      // LOW_CODEGEN::END::CUSTOM:SETTER_scene

      broadcast_observable(N(scene));
    }

    Low::Util::UniqueId Region::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(Region, unique_id, Low::Util::UniqueId);
    }
    void Region::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(Region, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

      broadcast_observable(N(unique_id));
    }

    Low::Util::Name Region::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Region, name, Low::Util::Name);
    }
    void Region::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Region> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Region, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    void Region::serialize_entities(Util::Yaml::Node &p_Node)
    {
      Low::Util::HandleLock<Region> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_serialize_entities

      for (auto it = get_entities().begin();
           it != get_entities().end(); ++it) {
        Core::Entity i_Entity =
            Util::find_handle_by_unique_id(*it).get_id();
        if (i_Entity.is_alive()) {
          Util::Yaml::Node i_Node;
          i_Entity.serialize(i_Node);
          p_Node["entities"].push_back(i_Node);
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_serialize_entities
    }

    void Region::add_entity(Entity p_Entity)
    {
      Low::Util::HandleLock<Region> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_entity

      if (p_Entity.get_region().is_alive()) {
        p_Entity.get_region().remove_entity(p_Entity);
      }

      p_Entity.set_region(*this);
      get_entities().insert(p_Entity.get_unique_id());
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_entity
    }

    void Region::remove_entity(Entity p_Entity)
    {
      Low::Util::HandleLock<Region> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_remove_entity

      p_Entity.set_region(0);
      get_entities().erase(p_Entity.get_unique_id());
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_remove_entity
    }

    void Region::load_entities()
    {
      Low::Util::HandleLock<Region> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_load_entities

      LOW_ASSERT(is_alive(), "Cannot load dead region");
      LOW_ASSERT(!is_loaded(), "Region is already loaded");

      set_loaded(true);

      Util::String l_Path =
          Util::get_project().dataPath + "\\assets\\regions\\";
      l_Path += std::to_string(get_unique_id()).c_str();
      l_Path += ".entities.yaml";

      if (!Util::FileIO::file_exists_sync(l_Path.c_str())) {
        return;
      }

      Util::Yaml::Node l_RootNode =
          Util::Yaml::load_file(l_Path.c_str());
      Util::Yaml::Node &l_EntitiesNode = l_RootNode["entities"];

      for (auto it = l_EntitiesNode.begin();
           it != l_EntitiesNode.end(); ++it) {
        Util::Yaml::Node &i_EntityNode = *it;
        Entity::deserialize(i_EntityNode, *this);
      }

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_load_entities
    }

    void Region::unload_entities()
    {
      Low::Util::HandleLock<Region> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_unload_entities

      LOW_ASSERT(is_alive(), "Cannot unload dead region");
      LOW_ASSERT(is_loaded(),
                 "Cannot unload region that is not loaded");

      set_loaded(false);

      while (!get_entities().empty()) {
        Entity i_Entity =
            Util::find_handle_by_unique_id(*get_entities().begin())
                .get_id();
        i_Entity.destroy();
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_unload_entities
    }

    uint32_t Region::create_instance(
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

    u32 Region::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for Region.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, Region::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool Region::get_page_for_index(const u32 p_Index,
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

  } // namespace Core
} // namespace Low
