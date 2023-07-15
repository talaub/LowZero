#include "LowCoreEntity.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowCoreTransform.h"
#include "LowCorePrefabInstance.h"

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Entity::TYPE_ID = 18;
    uint32_t Entity::ms_Capacity = 0u;
    uint8_t *Entity::ms_Buffer = 0;
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
      uint32_t l_Index = create_instance();

      Entity l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Entity::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Entity, components,
                              SINGLE_ARG(Util::Map<uint16_t, Util::Handle>)))
          Util::Map<uint16_t, Util::Handle>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Entity, region, Region)) Region();
      ACCESSOR_TYPE_SOA(l_Handle, Entity, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      l_Handle.set_unique_id(Low::Util::generate_unique_id(l_Handle.get_id()));
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
      for (auto it = get_components().begin(); it != get_components().end();
           ++it) {
        if (has_component(it->first)) {
          l_ComponentTypes.push_back(it->first);
        }
      }

      for (auto it = l_ComponentTypes.begin(); it != l_ComponentTypes.end();
           ++it) {
        Util::Handle i_Handle = get_component(*it);
        Util::RTTI::TypeInfo &i_TypeInfo = Util::Handle::get_type_info(*it);
        if (i_TypeInfo.is_alive(i_Handle)) {
          i_TypeInfo.destroy(i_Handle);
        }
      }

      if (get_region().is_alive()) {
        get_region().remove_entity(*this);
      }
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      Low::Util::remove_unique_id(get_unique_id());

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
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowCore), N(Entity));

      initialize_buffer(&ms_Buffer, EntityData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_Entity);
      LOW_PROFILE_ALLOC(type_slots_Entity);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Entity);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Entity::is_alive;
      l_TypeInfo.destroy = &Entity::destroy;
      l_TypeInfo.serialize = &Entity::serialize;
      l_TypeInfo.deserialize = &Entity::deserialize;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Entity::_make;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Entity::living_instances);
      l_TypeInfo.get_living_count = &Entity::living_count;
      l_TypeInfo.component = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(components);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(EntityData, components);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          Entity l_Handle = p_Handle.get_id();
          l_Handle.get_components();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Entity, components,
              SINGLE_ARG(Util::Map<uint16_t, Util::Handle>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(region);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(EntityData, region);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Region::TYPE_ID;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          Entity l_Handle = p_Handle.get_id();
          l_Handle.get_region();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Entity, region, Region);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Entity l_Handle = p_Handle.get_id();
          l_Handle.set_region(*(Region *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(unique_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(EntityData, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          Entity l_Handle = p_Handle.get_id();
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Entity, unique_id,
                                            Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(EntityData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
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
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Entity::cleanup()
    {
      Low::Util::List<Entity> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Entity);
      LOW_PROFILE_FREE(type_slots_Entity);
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
      return m_Data.m_Type == Entity::TYPE_ID &&
             check_alive(ms_Slots, Entity::get_capacity());
    }

    uint32_t Entity::get_capacity()
    {
      return ms_Capacity;
    }

    Entity Entity::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end();
           ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
    }

    void Entity::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      p_Node["name"] = get_name().c_str();
      p_Node["unique_id"] = get_unique_id();

      for (auto it = get_components().begin(); it != get_components().end();
           ++it) {
        Util::Yaml::Node i_Node;
        i_Node["type"] = it->first;

        Util::RTTI::TypeInfo &i_TypeInfo =
            Util::Handle::get_type_info(it->first);
        Util::Yaml::Node i_PropertiesNode;
        i_TypeInfo.serialize(it->second, i_PropertiesNode);
        i_Node["properties"] = i_PropertiesNode;
        p_Node["components"].push_back(i_Node);
      }
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Entity::serialize(Low::Util::Handle p_Handle,
                           Low::Util::Yaml::Node &p_Node)
    {
      Entity l_Entity = p_Handle.get_id();
      l_Entity.serialize(p_Node);
    }

    Low::Util::Handle Entity::deserialize(Low::Util::Yaml::Node &p_Node,
                                          Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      Region l_Region = p_Creator.get_id();

      Entity l_Entity = Entity::make(LOW_YAML_AS_NAME(p_Node["name"]));

      // Parse the old unique id and assign it again (need to remove the auto
      // generated uid first
      if (p_Node["unique_id"]) {
        Util::remove_unique_id(l_Entity.get_unique_id());
        l_Entity.set_unique_id(p_Node["unique_id"].as<uint64_t>());
        Util::register_unique_id(l_Entity.get_unique_id(), l_Entity);
      }

      l_Region.add_entity(l_Entity);

      Util::Yaml::Node &l_ComponentsNode = p_Node["components"];

      for (auto it = l_ComponentsNode.begin(); it != l_ComponentsNode.end();
           ++it) {
        Util::Yaml::Node &i_ComponentNode = *it;
        Util::RTTI::TypeInfo &i_TypeInfo =
            Util::Handle::get_type_info(i_ComponentNode["type"].as<uint16_t>());

        i_TypeInfo.deserialize(i_ComponentNode["properties"], l_Entity);
      }

      if (l_Entity.has_component(Component::PrefabInstance::TYPE_ID)) {
        Component::PrefabInstance l_PrefabInstance =
            l_Entity.get_component(Component::PrefabInstance::TYPE_ID);

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

      return TYPE_SOA(Entity, components,
                      SINGLE_ARG(Util::Map<uint16_t, Util::Handle>));
    }

    Region Entity::get_region() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_region
      // LOW_CODEGEN::END::CUSTOM:GETTER_region

      return TYPE_SOA(Entity, region, Region);
    }
    void Entity::set_region(Region p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_region
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_region

      // Set new value
      TYPE_SOA(Entity, region, Region) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_region
      // LOW_CODEGEN::END::CUSTOM:SETTER_region
    }

    Low::Util::UniqueId Entity::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(Entity, unique_id, Low::Util::UniqueId);
    }
    void Entity::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(Entity, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
    }

    Low::Util::Name Entity::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Entity, name, Low::Util::Name);
    }
    void Entity::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Entity, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    Entity Entity::make(Util::Name p_Name, Region p_Region)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      LOW_LOG_DEBUG << "Creating entity: " << p_Name << " with region"
                    << LOW_LOG_END;
      Entity l_Entity = Entity::make(p_Name);
      p_Region.add_entity(l_Entity);
      return l_Entity;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint64_t Entity::get_component(uint16_t p_TypeId)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_component
      if (get_components().find(p_TypeId) == get_components().end()) {
        return ~0ull;
      }
      return get_components()[p_TypeId].get_id();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_component
    }

    void Entity::add_component(Util::Handle &p_Component)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_component
      Util::Handle l_ExistingComponent = get_component(p_Component.get_type());
      Util::RTTI::TypeInfo l_ComponentTypeInfo =
          get_type_info(p_Component.get_type());

      LOW_ASSERT(l_ComponentTypeInfo.component,
                 "Can only add components to an entity");
      LOW_ASSERT(!l_ComponentTypeInfo.is_alive(l_ExistingComponent),
                 "An entity can only hold one component of a given type");

      l_ComponentTypeInfo.properties[N(entity)].set(p_Component, this);

      get_components()[p_Component.get_type()] = p_Component.get_id();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_component
    }

    void Entity::remove_component(uint16_t p_ComponentType)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_remove_component
      LOW_ASSERT(has_component(p_ComponentType),
                 "Cannot remove component from entity. This entity does not "
                 "have a component of the specified type");

      Util::RTTI::TypeInfo &l_TypeInfo =
          Util::Handle::get_type_info(p_ComponentType);

      l_TypeInfo.destroy(get_components()[p_ComponentType]);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_remove_component
    }

    bool Entity::has_component(uint16_t p_ComponentType)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_has_component
      if (get_components().find(p_ComponentType) == get_components().end()) {
        return false;
      }

      Util::Handle l_Handle = get_components()[p_ComponentType];

      Util::RTTI::TypeInfo &l_TypeInfo =
          Util::Handle::get_type_info(p_ComponentType);

      return l_TypeInfo.is_alive(l_Handle);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_has_component
    }

    Component::Transform Entity::get_transform()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_transform
      _LOW_ASSERT(is_alive());
      return get_component(Component::Transform::TYPE_ID);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_transform
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
      uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

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
          auto *i_ValPtr =
              new (&l_NewBuffer[offsetof(EntityData, components) *
                                    (l_Capacity + l_CapacityIncrease) +
                                (it->get_index() *
                                 sizeof(Util::Map<uint16_t, Util::Handle>))])
                  Util::Map<uint16_t, Util::Handle>();
          *i_ValPtr = it->get_components();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(EntityData, region) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(EntityData, region) * (l_Capacity)],
               l_Capacity * sizeof(Region));
      }
      {
        memcpy(&l_NewBuffer[offsetof(EntityData, unique_id) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(EntityData, unique_id) * (l_Capacity)],
               l_Capacity * sizeof(Low::Util::UniqueId));
      }
      {
        memcpy(&l_NewBuffer[offsetof(EntityData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(EntityData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for Entity from " << l_Capacity
                    << " to " << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }
  } // namespace Core
} // namespace Low
