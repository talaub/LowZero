#include "LowCoreEntity.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowCoreTransform.h"
#include "LowCorePrefabInstance.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Entity::TYPE_ID = 18;
    uint32_t Entity::ms_Capacity = 0u;
    uint8_t *Entity::ms_Buffer = 0;
    std::shared_mutex Entity::ms_BufferMutex;
    Low::Util::Instances::Slot *Entity::ms_Slots = 0;
    Low::Util::List<Entity> Entity::ms_LivingInstances =
        Low::Util::List<Entity>();

    Entity::Entity() : Low::Util::Handle(0ull)
    {
    }
    Entity::Entity(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Entity::Entity(Entity &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Entity::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Entity Entity::make(Low::Util::Name p_Name)
    {
      return make(p_Name, 0ull);
    }

    Entity Entity::make(Low::Util::Name p_Name,
                        Low::Util::UniqueId p_UniqueId)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      Entity l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Entity::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(
          l_Handle, Entity, components,
          SINGLE_ARG(Util::Map<uint16_t, Util::Handle>)))
          Util::Map<uint16_t, Util::Handle>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Entity, region, Region))
          Region();
      ACCESSOR_TYPE_SOA(l_Handle, Entity, name, Low::Util::Name) =
          Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

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

    void Entity::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

      Util::List<uint16_t> l_ComponentTypes;
      for (auto it = get_components().begin();
           it != get_components().end(); ++it) {
        if (has_component(it->first)) {
          l_ComponentTypes.push_back(it->first);
        }
      }

      for (auto it = l_ComponentTypes.begin();
           it != l_ComponentTypes.end(); ++it) {
        Util::Handle i_Handle = get_component(*it);
        Util::RTTI::TypeInfo &i_TypeInfo =
            Util::Handle::get_type_info(*it);
        if (i_TypeInfo.is_alive(i_Handle)) {
          i_TypeInfo.destroy(i_Handle);
        }
      }

      if (get_region().is_alive()) {
        get_region().remove_entity(*this);
      }
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      Low::Util::remove_unique_id(get_unique_id());

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const Entity *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void Entity::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowCore), N(Entity));

      initialize_buffer(&ms_Buffer, EntityData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_Entity);
      LOW_PROFILE_ALLOC(type_slots_Entity);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Entity);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Entity::is_alive;
      l_TypeInfo.destroy = &Entity::destroy;
      l_TypeInfo.serialize = &Entity::serialize;
      l_TypeInfo.deserialize = &Entity::deserialize;
      l_TypeInfo.find_by_index = &Entity::_find_by_index;
      l_TypeInfo.find_by_name = &Entity::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Entity::_make;
      l_TypeInfo.duplicate_default = &Entity::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Entity::living_instances);
      l_TypeInfo.get_living_count = &Entity::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: components
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(components);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(EntityData, components);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Entity l_Handle = p_Handle.get_id();
          l_Handle.get_components();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Entity, components,
              SINGLE_ARG(Util::Map<uint16_t, Util::Handle>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Entity l_Handle = p_Handle.get_id();
          *((Util::Map<uint16_t, Util::Handle> *)p_Data) =
              l_Handle.get_components();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: components
      }
      {
        // Property: region
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(region);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(EntityData, region);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Region::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Entity l_Handle = p_Handle.get_id();
          l_Handle.get_region();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Entity, region,
                                            Region);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Entity l_Handle = p_Handle.get_id();
          l_Handle.set_region(*(Region *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Entity l_Handle = p_Handle.get_id();
          *((Region *)p_Data) = l_Handle.get_region();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: region
      }
      {
        // Property: unique_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(unique_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(EntityData, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Entity l_Handle = p_Handle.get_id();
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Entity, unique_id, Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Entity l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(EntityData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Entity l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Entity, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Entity l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Entity l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: make
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = Entity::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Region);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = Region::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: get_component
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_component);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_TypeId);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UINT16;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_component
      }
      {
        // Function: add_component
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(add_component);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Component);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: add_component
      }
      {
        // Function: remove_component
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(remove_component);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_ComponentType);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UINT16;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: remove_component
      }
      {
        // Function: has_component
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(has_component);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_ComponentType);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UINT16;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: has_component
      }
      {
        // Function: get_transform
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_transform);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType =
            Low::Core::Component::Transform::TYPE_ID;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_transform
      }
      {
        // Function: serialize
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(serialize);
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
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_AddHandles);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: serialize
      }
      {
        // Function: serialize_hierarchy
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(serialize_hierarchy);
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
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_AddHandles);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: serialize_hierarchy
      }
      {
        // Function: deserialize_hierarchy
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(deserialize_hierarchy);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Node);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Creator);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: deserialize_hierarchy
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Entity::cleanup()
    {
      Low::Util::List<Entity> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Entity);
      LOW_PROFILE_FREE(type_slots_Entity);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle Entity::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    Entity Entity::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Entity l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = Entity::TYPE_ID;

      return l_Handle;
    }

    bool Entity::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == Entity::TYPE_ID &&
             check_alive(ms_Slots, Entity::get_capacity());
    }

    uint32_t Entity::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle Entity::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    Entity Entity::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    Entity Entity::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      Entity l_Entity = make(p_Name);

      for (auto it = get_components().begin();
           it != get_components().end(); ++it) {
        Util::RTTI::TypeInfo &i_ComponentTypeInfo =
            Util::Handle::get_type_info(it->first);

        i_ComponentTypeInfo.duplicate_component(it->second, l_Entity);
      }

      Component::Transform l_Transform = get_transform();

      for (u32 i = 0; i < l_Transform.get_children().size(); ++i) {
        Component::Transform i_ChildTransform =
            l_Transform.get_children()[i];

        Entity i_CopiedEntity =
            i_ChildTransform.get_entity().duplicate(
                i_ChildTransform.get_entity().get_name());

        i_CopiedEntity.get_transform().set_parent(
            l_Entity.get_transform());
      }

      l_Entity.get_transform().set_parent(l_Transform.get_parent());
      get_region().add_entity(l_Entity);

      return l_Entity;
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE
    }

    Entity Entity::duplicate(Entity p_Handle, Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle Entity::_duplicate(Low::Util::Handle p_Handle,
                                         Low::Util::Name p_Name)
    {
      Entity l_Entity = p_Handle.get_id();
      return l_Entity.duplicate(p_Name);
    }

    void Entity::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      serialize(p_Node, false);
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Entity::serialize(Low::Util::Handle p_Handle,
                           Low::Util::Yaml::Node &p_Node)
    {
      Entity l_Entity = p_Handle.get_id();
      l_Entity.serialize(p_Node);
    }

    Low::Util::Handle
    Entity::deserialize(Low::Util::Yaml::Node &p_Node,
                        Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      Region l_Region = p_Creator.get_id();

      if (!l_Region.is_alive()) {
        if (p_Node["region"]) {
          l_Region = Util::find_handle_by_unique_id(
                         p_Node["region"].as<uint64_t>())
                         .get_id();
        }
      }

      Entity l_Entity =
          Entity::make(LOW_YAML_AS_NAME(p_Node["name"]));

      p_Node["_handle"] = l_Entity.get_id();

      // Parse the old unique id and assign it again (need to
      // remove the auto generated uid first)
      if (p_Node["unique_id"]) {
        Util::remove_unique_id(l_Entity.get_unique_id());
        l_Entity.set_unique_id(p_Node["unique_id"].as<uint64_t>());
        Util::register_unique_id(l_Entity.get_unique_id(), l_Entity);
      }

      l_Region.add_entity(l_Entity);

      Util::Yaml::Node &l_ComponentsNode = p_Node["components"];

      for (auto it = l_ComponentsNode.begin();
           it != l_ComponentsNode.end(); ++it) {
        Util::Yaml::Node &i_ComponentNode = *it;
        Util::RTTI::TypeInfo &i_TypeInfo =
            Util::Handle::get_type_info(
                i_ComponentNode["type"].as<uint16_t>());

        i_ComponentNode["_handle"] =
            i_TypeInfo
                .deserialize(i_ComponentNode["properties"], l_Entity)
                .get_id();
      }

      if (l_Entity.has_component(
              Component::PrefabInstance::TYPE_ID)) {
        Component::PrefabInstance l_PrefabInstance =
            l_Entity.get_component(
                Component::PrefabInstance::TYPE_ID);

        l_PrefabInstance.update_from_prefab();
      }

      return l_Entity;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    Util::Map<uint16_t, Util::Handle> &Entity::get_components() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_components

      // LOW_CODEGEN::END::CUSTOM:GETTER_components

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Entity, components,
                      SINGLE_ARG(Util::Map<uint16_t, Util::Handle>));
    }

    Region Entity::get_region() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_region

      // LOW_CODEGEN::END::CUSTOM:GETTER_region

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Entity, region, Region);
    }
    void Entity::set_region(Region p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_region

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_region

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Entity, region, Region) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_region

      // LOW_CODEGEN::END::CUSTOM:SETTER_region
    }

    Low::Util::UniqueId Entity::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Entity, unique_id, Low::Util::UniqueId);
    }
    void Entity::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Entity, unique_id, Low::Util::UniqueId) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
    }

    Low::Util::Name Entity::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Entity, name, Low::Util::Name);
    }
    void Entity::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Entity, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    Entity Entity::make(Util::Name p_Name, Region p_Region)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make

      Entity l_Entity = Entity::make(p_Name);
      p_Region.add_entity(l_Entity);
      return l_Entity;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint64_t Entity::get_component(uint16_t p_TypeId) const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_component

      if (get_components().find(p_TypeId) == get_components().end()) {
        return ~0ull;
      }
      return get_components()[p_TypeId].get_id();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_component
    }

    void Entity::add_component(Low::Util::Handle &p_Component)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_component

      Util::Handle l_ExistingComponent =
          get_component(p_Component.get_type());
      Util::RTTI::TypeInfo l_ComponentTypeInfo =
          get_type_info(p_Component.get_type());

      LOW_ASSERT(l_ComponentTypeInfo.component,
                 "Can only add components to an entity");
      LOW_ASSERT(!l_ComponentTypeInfo.is_alive(l_ExistingComponent),
                 "An entity can only hold one component of a given "
                 "type");

      l_ComponentTypeInfo.properties[N(entity)].set(p_Component,
                                                    this);

      get_components()[p_Component.get_type()] = p_Component.get_id();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_component
    }

    void Entity::remove_component(uint16_t p_ComponentType)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_remove_component

      LOW_ASSERT(has_component(p_ComponentType),
                 "Cannot remove component from entity. This "
                 "entity does not "
                 "have a component of the specified type");

      Util::RTTI::TypeInfo &l_TypeInfo =
          Util::Handle::get_type_info(p_ComponentType);

      l_TypeInfo.destroy(get_components()[p_ComponentType]);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_remove_component
    }

    bool Entity::has_component(uint16_t p_ComponentType)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_has_component

      if (get_components().find(p_ComponentType) ==
          get_components().end()) {
        return false;
      }

      Util::Handle l_Handle = get_components()[p_ComponentType];

      Util::RTTI::TypeInfo &l_TypeInfo =
          Util::Handle::get_type_info(p_ComponentType);

      return l_TypeInfo.is_alive(l_Handle);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_has_component
    }

    Low::Core::Component::Transform Entity::get_transform() const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_transform

      _LOW_ASSERT(is_alive());
      return get_component(Component::Transform::TYPE_ID);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_transform
    }

    void Entity::serialize(Low::Util::Yaml::Node &p_Node,
                           bool p_AddHandles) const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_serialize

      _LOW_ASSERT(is_alive());

      p_Node["name"] = get_name().c_str();
      p_Node["unique_id"] = get_unique_id();
      if (p_AddHandles) {
        p_Node["handle"] = get_id();
      }
      p_Node["region"] = 0;
      if (get_region().is_alive()) {
        p_Node["region"] = get_region().get_unique_id();
      }

      for (auto it = get_components().begin();
           it != get_components().end(); ++it) {
        Util::Yaml::Node i_Node;
        i_Node["type"] = it->first;
        if (p_AddHandles) {
          i_Node["handle"] = it->second.get_id();
        }

        Util::RTTI::TypeInfo &i_TypeInfo =
            Util::Handle::get_type_info(it->first);
        Util::Yaml::Node i_PropertiesNode;
        i_TypeInfo.serialize(it->second, i_PropertiesNode);
        i_Node["properties"] = i_PropertiesNode;
        p_Node["components"].push_back(i_Node);
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_serialize
    }

    void Entity::serialize_hierarchy(Util::Yaml::Node &p_Node,
                                     bool p_AddHandles) const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_serialize_hierarchy

      serialize(p_Node, p_AddHandles);

      Component::Transform l_Transform = get_transform();

      for (auto it = l_Transform.get_children().begin();
           it != l_Transform.get_children().end(); ++it) {
        Component::Transform i_Child = *it;

        Util::Yaml::Node i_Node;
        i_Child.get_entity().serialize_hierarchy(i_Node,
                                                 p_AddHandles);

        p_Node["children"].push_back(i_Node);
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_serialize_hierarchy
    }

    Entity &Entity::deserialize_hierarchy(Util::Yaml::Node &p_Node,
                                          Low::Util::Handle p_Creator)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_deserialize_hierarchy

      Entity l_Entity =
          (Entity)deserialize(p_Node, p_Creator).get_id();

      if (p_Node["children"]) {
        for (uint32_t i = 0; i < p_Node["children"].size(); ++i) {
          Entity::deserialize_hierarchy(p_Node["children"][i],
                                        p_Creator);
        }
      }

      return l_Entity;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_deserialize_hierarchy
    }

    uint32_t Entity::create_instance()
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

    void Entity::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(EntityData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          Entity i_Entity = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(EntityData, components) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(
                                Util::Map<uint16_t, Util::Handle>))])
              Util::Map<uint16_t, Util::Handle>();
          *i_ValPtr = ACCESSOR_TYPE_SOA(
              i_Entity, Entity, components,
              SINGLE_ARG(Util::Map<uint16_t, Util::Handle>));
        }
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(EntityData, region) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(EntityData, region) * (l_Capacity)],
            l_Capacity * sizeof(Region));
      }
      {
        memcpy(&l_NewBuffer[offsetof(EntityData, unique_id) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(EntityData, unique_id) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Util::UniqueId));
      }
      {
        memcpy(&l_NewBuffer[offsetof(EntityData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(EntityData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for Entity from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Core
} // namespace Low
