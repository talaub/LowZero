#include "LowCoreEntity.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Core {
    const uint16_t Entity::TYPE_ID = 1;
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
      ms_Capacity = Low::Util::Config::get_capacity(N(LowCore), N(Entity));

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
      return ms_Capacity;
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
        memcpy(&l_NewBuffer[offsetof(EntityData, components) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(EntityData, components) * (l_Capacity)],
               l_Capacity * sizeof(Util::Map<uint16_t, Util::Handle>));
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
