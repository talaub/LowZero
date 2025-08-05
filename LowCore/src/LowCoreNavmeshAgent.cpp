#include "LowCoreNavmeshAgent.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowCorePrefabInstance.h"
#include "LowCoreTransform.h"
#include "LowCoreNavmeshSystem.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t NavmeshAgent::TYPE_ID = 35;
      uint32_t NavmeshAgent::ms_Capacity = 0u;
      uint8_t *NavmeshAgent::ms_Buffer = 0;
      std::shared_mutex NavmeshAgent::ms_BufferMutex;
      Low::Util::Instances::Slot *NavmeshAgent::ms_Slots = 0;
      Low::Util::List<NavmeshAgent> NavmeshAgent::ms_LivingInstances =
          Low::Util::List<NavmeshAgent>();

      NavmeshAgent::NavmeshAgent() : Low::Util::Handle(0ull)
      {
      }
      NavmeshAgent::NavmeshAgent(uint64_t p_Id)
          : Low::Util::Handle(p_Id)
      {
      }
      NavmeshAgent::NavmeshAgent(NavmeshAgent &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle
      NavmeshAgent::_make(Low::Util::Handle p_Entity)
      {
        Low::Core::Entity l_Entity = p_Entity.get_id();
        LOW_ASSERT(l_Entity.is_alive(),
                   "Cannot create component for dead entity");
        return make(l_Entity).get_id();
      }

      NavmeshAgent NavmeshAgent::make(Low::Core::Entity p_Entity)
      {
        return make(p_Entity, 0ull);
      }

      NavmeshAgent NavmeshAgent::make(Low::Core::Entity p_Entity,
                                      Low::Util::UniqueId p_UniqueId)
      {
        WRITE_LOCK(l_Lock);
        uint32_t l_Index = create_instance();

        NavmeshAgent l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = NavmeshAgent::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, NavmeshAgent, speed, float) =
            0.0f;
        ACCESSOR_TYPE_SOA(l_Handle, NavmeshAgent, height, float) =
            0.0f;
        ACCESSOR_TYPE_SOA(l_Handle, NavmeshAgent, radius, float) =
            0.0f;
        new (&ACCESSOR_TYPE_SOA(l_Handle, NavmeshAgent, offset,
                                Low::Math::Vector3))
            Low::Math::Vector3();
        new (&ACCESSOR_TYPE_SOA(l_Handle, NavmeshAgent, entity,
                                Low::Core::Entity))
            Low::Core::Entity();
        LOCK_UNLOCK(l_Lock);

        l_Handle.set_entity(p_Entity);
        p_Entity.add_component(l_Handle);

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

        l_Handle.set_agent_index(-1);
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void NavmeshAgent::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        // LOW_CODEGEN::END::CUSTOM:DESTROY

        broadcast_observable(OBSERVABLE_DESTROY);

        Low::Util::remove_unique_id(get_unique_id());

        WRITE_LOCK(l_Lock);
        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const NavmeshAgent *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
      }

      void NavmeshAgent::initialize()
      {
        WRITE_LOCK(l_Lock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(
            N(LowCore), N(NavmeshAgent));

        initialize_buffer(&ms_Buffer, NavmeshAgentData::get_size(),
                          get_capacity(), &ms_Slots);
        LOCK_UNLOCK(l_Lock);

        LOW_PROFILE_ALLOC(type_buffer_NavmeshAgent);
        LOW_PROFILE_ALLOC(type_slots_NavmeshAgent);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(NavmeshAgent);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &NavmeshAgent::is_alive;
        l_TypeInfo.destroy = &NavmeshAgent::destroy;
        l_TypeInfo.serialize = &NavmeshAgent::serialize;
        l_TypeInfo.deserialize = &NavmeshAgent::deserialize;
        l_TypeInfo.find_by_index = &NavmeshAgent::_find_by_index;
        l_TypeInfo.notify = &NavmeshAgent::_notify;
        l_TypeInfo.make_default = nullptr;
        l_TypeInfo.make_component = &NavmeshAgent::_make;
        l_TypeInfo.duplicate_default = nullptr;
        l_TypeInfo.duplicate_component = &NavmeshAgent::_duplicate;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &NavmeshAgent::living_instances);
        l_TypeInfo.get_living_count = &NavmeshAgent::living_count;
        l_TypeInfo.component = true;
        l_TypeInfo.uiComponent = false;
        {
          // Property: speed
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(speed);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(NavmeshAgentData, speed);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            NavmeshAgent l_Handle = p_Handle.get_id();
            l_Handle.get_speed();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, NavmeshAgent,
                                              speed, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            NavmeshAgent l_Handle = p_Handle.get_id();
            l_Handle.set_speed(*(float *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            NavmeshAgent l_Handle = p_Handle.get_id();
            *((float *)p_Data) = l_Handle.get_speed();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: speed
        }
        {
          // Property: height
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(height);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(NavmeshAgentData, height);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            NavmeshAgent l_Handle = p_Handle.get_id();
            l_Handle.get_height();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, NavmeshAgent,
                                              height, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            NavmeshAgent l_Handle = p_Handle.get_id();
            l_Handle.set_height(*(float *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            NavmeshAgent l_Handle = p_Handle.get_id();
            *((float *)p_Data) = l_Handle.get_height();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: height
        }
        {
          // Property: radius
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(radius);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(NavmeshAgentData, radius);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            NavmeshAgent l_Handle = p_Handle.get_id();
            l_Handle.get_radius();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, NavmeshAgent,
                                              radius, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            NavmeshAgent l_Handle = p_Handle.get_id();
            l_Handle.set_radius(*(float *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            NavmeshAgent l_Handle = p_Handle.get_id();
            *((float *)p_Data) = l_Handle.get_radius();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: radius
        }
        {
          // Property: offset
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(offset);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(NavmeshAgentData, offset);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            NavmeshAgent l_Handle = p_Handle.get_id();
            l_Handle.get_offset();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, NavmeshAgent, offset, Low::Math::Vector3);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            NavmeshAgent l_Handle = p_Handle.get_id();
            l_Handle.set_offset(*(Low::Math::Vector3 *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            NavmeshAgent l_Handle = p_Handle.get_id();
            *((Low::Math::Vector3 *)p_Data) = l_Handle.get_offset();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: offset
        }
        {
          // Property: agent_index
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(agent_index);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(NavmeshAgentData, agent_index);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::INT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            NavmeshAgent l_Handle = p_Handle.get_id();
            l_Handle.get_agent_index();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, NavmeshAgent,
                                              agent_index, int);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            NavmeshAgent l_Handle = p_Handle.get_id();
            l_Handle.set_agent_index(*(int *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            NavmeshAgent l_Handle = p_Handle.get_id();
            *((int *)p_Data) = l_Handle.get_agent_index();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: agent_index
        }
        {
          // Property: entity
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(NavmeshAgentData, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::TYPE_ID;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            NavmeshAgent l_Handle = p_Handle.get_id();
            l_Handle.get_entity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, NavmeshAgent, entity, Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            NavmeshAgent l_Handle = p_Handle.get_id();
            l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            NavmeshAgent l_Handle = p_Handle.get_id();
            *((Low::Core::Entity *)p_Data) = l_Handle.get_entity();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: entity
        }
        {
          // Property: unique_id
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(NavmeshAgentData, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            NavmeshAgent l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, NavmeshAgent,
                                              unique_id,
                                              Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            NavmeshAgent l_Handle = p_Handle.get_id();
            *((Low::Util::UniqueId *)p_Data) =
                l_Handle.get_unique_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: unique_id
        }
        {
          // Function: set_target_position
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(set_target_position);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_TargetPosition);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR3;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: set_target_position
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void NavmeshAgent::cleanup()
      {
        Low::Util::List<NavmeshAgent> l_Instances =
            ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        WRITE_LOCK(l_Lock);
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_NavmeshAgent);
        LOW_PROFILE_FREE(type_slots_NavmeshAgent);
        LOCK_UNLOCK(l_Lock);
      }

      Low::Util::Handle NavmeshAgent::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      NavmeshAgent NavmeshAgent::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        NavmeshAgent l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = NavmeshAgent::TYPE_ID;

        return l_Handle;
      }

      bool NavmeshAgent::is_alive() const
      {
        READ_LOCK(l_Lock);
        return m_Data.m_Type == NavmeshAgent::TYPE_ID &&
               check_alive(ms_Slots, NavmeshAgent::get_capacity());
      }

      uint32_t NavmeshAgent::get_capacity()
      {
        return ms_Capacity;
      }

      NavmeshAgent
      NavmeshAgent::duplicate(Low::Core::Entity p_Entity) const
      {
        _LOW_ASSERT(is_alive());

        NavmeshAgent l_Handle = make(p_Entity);
        l_Handle.set_speed(get_speed());
        l_Handle.set_height(get_height());
        l_Handle.set_radius(get_radius());
        l_Handle.set_offset(get_offset());
        l_Handle.set_agent_index(get_agent_index());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      NavmeshAgent NavmeshAgent::duplicate(NavmeshAgent p_Handle,
                                           Low::Core::Entity p_Entity)
      {
        return p_Handle.duplicate(p_Entity);
      }

      Low::Util::Handle
      NavmeshAgent::_duplicate(Low::Util::Handle p_Handle,
                               Low::Util::Handle p_Entity)
      {
        NavmeshAgent l_NavmeshAgent = p_Handle.get_id();
        Low::Core::Entity l_Entity = p_Entity.get_id();
        return l_NavmeshAgent.duplicate(l_Entity);
      }

      void
      NavmeshAgent::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["speed"] = get_speed();
        p_Node["height"] = get_height();
        p_Node["radius"] = get_radius();
        Low::Util::Serialization::serialize(p_Node["offset"],
                                            get_offset());
        p_Node["agent_index"] = get_agent_index();
        p_Node["unique_id"] = get_unique_id();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void NavmeshAgent::serialize(Low::Util::Handle p_Handle,
                                   Low::Util::Yaml::Node &p_Node)
      {
        NavmeshAgent l_NavmeshAgent = p_Handle.get_id();
        l_NavmeshAgent.serialize(p_Node);
      }

      Low::Util::Handle
      NavmeshAgent::deserialize(Low::Util::Yaml::Node &p_Node,
                                Low::Util::Handle p_Creator)
      {
        NavmeshAgent l_Handle =
            NavmeshAgent::make(p_Creator.get_id());

        if (p_Node["unique_id"]) {
          Low::Util::remove_unique_id(l_Handle.get_unique_id());
          l_Handle.set_unique_id(p_Node["unique_id"].as<uint64_t>());
          Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                        l_Handle.get_id());
        }

        if (p_Node["speed"]) {
          l_Handle.set_speed(p_Node["speed"].as<float>());
        }
        if (p_Node["height"]) {
          l_Handle.set_height(p_Node["height"].as<float>());
        }
        if (p_Node["radius"]) {
          l_Handle.set_radius(p_Node["radius"].as<float>());
        }
        if (p_Node["offset"]) {
          l_Handle.set_offset(
              Low::Util::Serialization::deserialize_vector3(
                  p_Node["offset"]));
        }
        if (p_Node["agent_index"]) {
          l_Handle.set_agent_index(p_Node["agent_index"].as<int>());
        }
        if (p_Node["unique_id"]) {
          l_Handle.set_unique_id(
              p_Node["unique_id"].as<Low::Util::UniqueId>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void NavmeshAgent::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 NavmeshAgent::observe(Low::Util::Name p_Observable,
                                Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void NavmeshAgent::notify(Low::Util::Handle p_Observed,
                                Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void NavmeshAgent::_notify(Low::Util::Handle p_Observer,
                                 Low::Util::Handle p_Observed,
                                 Low::Util::Name p_Observable)
      {
        NavmeshAgent l_NavmeshAgent = p_Observer.get_id();
        l_NavmeshAgent.notify(p_Observed, p_Observable);
      }

      float NavmeshAgent::get_speed() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_speed

        // LOW_CODEGEN::END::CUSTOM:GETTER_speed

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(NavmeshAgent, speed, float);
      }
      void NavmeshAgent::set_speed(float p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_speed

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_speed

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(NavmeshAgent, speed, float) = p_Value;
        LOCK_UNLOCK(l_WriteLock);
        {
          Low::Core::Entity l_Entity = get_entity();
          if (l_Entity.has_component(
                  Low::Core::Component::PrefabInstance::TYPE_ID)) {
            Low::Core::Component::PrefabInstance l_Instance =
                l_Entity.get_component(
                    Low::Core::Component::PrefabInstance::TYPE_ID);
            Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
            if (l_Prefab.is_alive()) {
              l_Instance.override(
                  TYPE_ID, N(speed),
                  !l_Prefab.compare_property(*this, N(speed)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_speed

        // LOW_CODEGEN::END::CUSTOM:SETTER_speed

        broadcast_observable(N(speed));
      }

      float NavmeshAgent::get_height() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_height

        // LOW_CODEGEN::END::CUSTOM:GETTER_height

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(NavmeshAgent, height, float);
      }
      void NavmeshAgent::set_height(float p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_height

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_height

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(NavmeshAgent, height, float) = p_Value;
        LOCK_UNLOCK(l_WriteLock);
        {
          Low::Core::Entity l_Entity = get_entity();
          if (l_Entity.has_component(
                  Low::Core::Component::PrefabInstance::TYPE_ID)) {
            Low::Core::Component::PrefabInstance l_Instance =
                l_Entity.get_component(
                    Low::Core::Component::PrefabInstance::TYPE_ID);
            Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
            if (l_Prefab.is_alive()) {
              l_Instance.override(
                  TYPE_ID, N(height),
                  !l_Prefab.compare_property(*this, N(height)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_height

        // LOW_CODEGEN::END::CUSTOM:SETTER_height

        broadcast_observable(N(height));
      }

      float NavmeshAgent::get_radius() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_radius

        // LOW_CODEGEN::END::CUSTOM:GETTER_radius

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(NavmeshAgent, radius, float);
      }
      void NavmeshAgent::set_radius(float p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_radius

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_radius

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(NavmeshAgent, radius, float) = p_Value;
        LOCK_UNLOCK(l_WriteLock);
        {
          Low::Core::Entity l_Entity = get_entity();
          if (l_Entity.has_component(
                  Low::Core::Component::PrefabInstance::TYPE_ID)) {
            Low::Core::Component::PrefabInstance l_Instance =
                l_Entity.get_component(
                    Low::Core::Component::PrefabInstance::TYPE_ID);
            Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
            if (l_Prefab.is_alive()) {
              l_Instance.override(
                  TYPE_ID, N(radius),
                  !l_Prefab.compare_property(*this, N(radius)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_radius

        // LOW_CODEGEN::END::CUSTOM:SETTER_radius

        broadcast_observable(N(radius));
      }

      Low::Math::Vector3 &NavmeshAgent::get_offset() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_offset

        // LOW_CODEGEN::END::CUSTOM:GETTER_offset

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(NavmeshAgent, offset, Low::Math::Vector3);
      }
      void NavmeshAgent::set_offset(float p_X, float p_Y, float p_Z)
      {
        Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
        set_offset(p_Val);
      }

      void NavmeshAgent::set_offset_x(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_offset();
        l_Value.x = p_Value;
        set_offset(l_Value);
      }

      void NavmeshAgent::set_offset_y(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_offset();
        l_Value.y = p_Value;
        set_offset(l_Value);
      }

      void NavmeshAgent::set_offset_z(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_offset();
        l_Value.z = p_Value;
        set_offset(l_Value);
      }

      void NavmeshAgent::set_offset(Low::Math::Vector3 &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_offset

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_offset

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(NavmeshAgent, offset, Low::Math::Vector3) = p_Value;
        LOCK_UNLOCK(l_WriteLock);
        {
          Low::Core::Entity l_Entity = get_entity();
          if (l_Entity.has_component(
                  Low::Core::Component::PrefabInstance::TYPE_ID)) {
            Low::Core::Component::PrefabInstance l_Instance =
                l_Entity.get_component(
                    Low::Core::Component::PrefabInstance::TYPE_ID);
            Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
            if (l_Prefab.is_alive()) {
              l_Instance.override(
                  TYPE_ID, N(offset),
                  !l_Prefab.compare_property(*this, N(offset)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_offset

        // LOW_CODEGEN::END::CUSTOM:SETTER_offset

        broadcast_observable(N(offset));
      }

      int NavmeshAgent::get_agent_index() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_agent_index

        // LOW_CODEGEN::END::CUSTOM:GETTER_agent_index

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(NavmeshAgent, agent_index, int);
      }
      void NavmeshAgent::set_agent_index(int p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_agent_index

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_agent_index

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(NavmeshAgent, agent_index, int) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_agent_index

        // LOW_CODEGEN::END::CUSTOM:SETTER_agent_index

        broadcast_observable(N(agent_index));
      }

      Low::Core::Entity NavmeshAgent::get_entity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity

        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(NavmeshAgent, entity, Low::Core::Entity);
      }
      void NavmeshAgent::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(NavmeshAgent, entity, Low::Core::Entity) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity

        // LOW_CODEGEN::END::CUSTOM:SETTER_entity

        broadcast_observable(N(entity));
      }

      Low::Util::UniqueId NavmeshAgent::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(NavmeshAgent, unique_id, Low::Util::UniqueId);
      }
      void NavmeshAgent::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(NavmeshAgent, unique_id, Low::Util::UniqueId) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

        broadcast_observable(N(unique_id));
      }

      void NavmeshAgent::set_target_position(
          Low::Math::Vector3 &p_TargetPosition)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_target_position

        System::Navmesh::set_agent_target_position(get_id(),
                                                   p_TargetPosition);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_target_position
      }

      uint32_t NavmeshAgent::create_instance()
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

      void NavmeshAgent::increase_budget()
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
                              sizeof(NavmeshAgentData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(&l_NewBuffer[offsetof(NavmeshAgentData, speed) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(NavmeshAgentData, speed) *
                            (l_Capacity)],
                 l_Capacity * sizeof(float));
        }
        {
          memcpy(&l_NewBuffer[offsetof(NavmeshAgentData, height) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(NavmeshAgentData, height) *
                            (l_Capacity)],
                 l_Capacity * sizeof(float));
        }
        {
          memcpy(&l_NewBuffer[offsetof(NavmeshAgentData, radius) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(NavmeshAgentData, radius) *
                            (l_Capacity)],
                 l_Capacity * sizeof(float));
        }
        {
          memcpy(&l_NewBuffer[offsetof(NavmeshAgentData, offset) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(NavmeshAgentData, offset) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Math::Vector3));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(NavmeshAgentData, agent_index) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(NavmeshAgentData, agent_index) *
                         (l_Capacity)],
              l_Capacity * sizeof(int));
        }
        {
          memcpy(&l_NewBuffer[offsetof(NavmeshAgentData, entity) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(NavmeshAgentData, entity) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Core::Entity));
        }
        {
          memcpy(&l_NewBuffer[offsetof(NavmeshAgentData, unique_id) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(NavmeshAgentData, unique_id) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Util::UniqueId));
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

        LOW_LOG_DEBUG
            << "Auto-increased budget for NavmeshAgent from "
            << l_Capacity << " to "
            << (l_Capacity + l_CapacityIncrease) << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Component
  }   // namespace Core
} // namespace Low
