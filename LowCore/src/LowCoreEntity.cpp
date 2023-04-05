#include "LowCoreEntity.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Core {
    const uint16_t Entity::TYPE_ID = 1;
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

    Entity Entity::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = Low::Util::Instances::create_instance(
          ms_Buffer, ms_Slots, get_capacity());

      Entity l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Entity::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Entity, components,
                              SINGLE_ARG(Util::Map<uint16_t, Util::Handle>)))
          Util::Map<uint16_t, Util::Handle>();
      ACCESSOR_TYPE_SOA(l_Handle, Entity, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      return l_Handle;
    }

    void Entity::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

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
      _LOW_ASSERT(l_LivingInstanceFound);
    }

    void Entity::initialize()
    {
      initialize_buffer(&ms_Buffer, EntityData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_Entity);
      LOW_PROFILE_ALLOC(type_slots_Entity);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Entity);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Entity::is_alive;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(components);
        l_PropertyInfo.dataOffset = offsetof(EntityData, components);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Entity, components,
              SINGLE_ARG(Util::Map<uint16_t, Util::Handle>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, Entity, components,
                            SINGLE_ARG(Util::Map<uint16_t, Util::Handle>)) =
              *(Util::Map<uint16_t, Util::Handle> *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.dataOffset = offsetof(EntityData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Entity, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, Entity, name, Low::Util::Name) =
              *(Low::Util::Name *)p_Data;
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

    bool Entity::is_alive() const
    {
      return m_Data.m_Type == Entity::TYPE_ID &&
             check_alive(ms_Slots, Entity::get_capacity());
    }

    uint32_t Entity::get_capacity()
    {
      static uint32_t l_Capacity = 0u;
      if (l_Capacity == 0u) {
        l_Capacity = Low::Util::Config::get_capacity(N(LowCore), N(Entity));
      }
      return l_Capacity;
    }

    Util::Map<uint16_t, Util::Handle> &Entity::get_components() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Entity, components,
                      SINGLE_ARG(Util::Map<uint16_t, Util::Handle>));
    }

    Low::Util::Name Entity::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Entity, name, Low::Util::Name);
    }
    void Entity::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(Entity, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    uint64_t Entity::get_component(uint16_t p_TypeId)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_component
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_component
    }

  } // namespace Core
} // namespace Low
