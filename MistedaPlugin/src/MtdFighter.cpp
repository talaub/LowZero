#include "MtdFighter.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowCorePrefabInstance.h"
// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include <cstdlib>
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Mtd {
  namespace Component {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Fighter::TYPE_ID = 46;
    uint32_t Fighter::ms_Capacity = 0u;
    uint8_t *Fighter::ms_Buffer = 0;
    Low::Util::Instances::Slot *Fighter::ms_Slots = 0;
    Low::Util::List<Fighter> Fighter::ms_LivingInstances =
        Low::Util::List<Fighter>();

    Fighter::Fighter() : Low::Util::Handle(0ull)
    {
    }
    Fighter::Fighter(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Fighter::Fighter(Fighter &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Fighter::_make(Low::Util::Handle p_Entity)
    {
      Low::Core::Entity l_Entity = p_Entity.get_id();
      LOW_ASSERT(l_Entity.is_alive(),
                 "Cannot create component for dead entity");
      return make(l_Entity).get_id();
    }

    Fighter Fighter::make(Low::Core::Entity p_Entity)
    {
      uint32_t l_Index = create_instance();

      Fighter l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Fighter::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Fighter, deck,
                              Low::Util::List<Mtd::Ability>))
          Low::Util::List<Mtd::Ability>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Fighter, entity,
                              Low::Core::Entity)) Low::Core::Entity();

      l_Handle.set_entity(p_Entity);
      p_Entity.add_component(l_Handle);

      ms_LivingInstances.push_back(l_Handle);

      l_Handle.set_unique_id(
          Low::Util::generate_unique_id(l_Handle.get_id()));
      Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                    l_Handle.get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      for (int i = 0; i < 15; ++i) {
        int l_RandomNumber = std::rand() % (Ability::living_count());
        l_Handle.get_deck().push_back(
            Ability::living_instances()[l_RandomNumber]);
      }

      l_Handle.set_mana(0);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Fighter::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      Low::Util::remove_unique_id(get_unique_id());

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const Fighter *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void Fighter::initialize()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(MistedaPlugin),
                                                    N(Fighter));

      initialize_buffer(&ms_Buffer, FighterData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_Fighter);
      LOW_PROFILE_ALLOC(type_slots_Fighter);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Fighter);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Fighter::is_alive;
      l_TypeInfo.destroy = &Fighter::destroy;
      l_TypeInfo.serialize = &Fighter::serialize;
      l_TypeInfo.deserialize = &Fighter::deserialize;
      l_TypeInfo.make_default = nullptr;
      l_TypeInfo.make_component = &Fighter::_make;
      l_TypeInfo.duplicate_default = nullptr;
      l_TypeInfo.duplicate_component = &Fighter::_duplicate;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Fighter::living_instances);
      l_TypeInfo.get_living_count = &Fighter::living_count;
      l_TypeInfo.component = true;
      l_TypeInfo.uiComponent = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(deck);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(FighterData, deck);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          Fighter l_Handle = p_Handle.get_id();
          l_Handle.get_deck();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Fighter, deck, Low::Util::List<Mtd::Ability>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Fighter l_Handle = p_Handle.get_id();
          l_Handle.set_deck(*(Low::Util::List<Mtd::Ability> *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(mana);
        l_PropertyInfo.editorProperty = true;
        l_PropertyInfo.dataOffset = offsetof(FighterData, mana);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          Fighter l_Handle = p_Handle.get_id();
          l_Handle.get_mana();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Fighter, mana,
                                            uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Fighter l_Handle = p_Handle.get_id();
          l_Handle.set_mana(*(uint32_t *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(entity);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(FighterData, entity);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Core::Entity::TYPE_ID;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          Fighter l_Handle = p_Handle.get_id();
          l_Handle.get_entity();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Fighter, entity,
                                            Low::Core::Entity);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Fighter l_Handle = p_Handle.get_id();
          l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(unique_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(FighterData, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          Fighter l_Handle = p_Handle.get_id();
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Fighter, unique_id, Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Fighter::cleanup()
    {
      Low::Util::List<Fighter> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Fighter);
      LOW_PROFILE_FREE(type_slots_Fighter);
    }

    Fighter Fighter::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Fighter l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = Fighter::TYPE_ID;

      return l_Handle;
    }

    bool Fighter::is_alive() const
    {
      return m_Data.m_Type == Fighter::TYPE_ID &&
             check_alive(ms_Slots, Fighter::get_capacity());
    }

    uint32_t Fighter::get_capacity()
    {
      return ms_Capacity;
    }

    Fighter Fighter::duplicate(Low::Core::Entity p_Entity) const
    {
      _LOW_ASSERT(is_alive());

      Fighter l_Handle = make(p_Entity);
      l_Handle.set_deck(get_deck());
      l_Handle.set_mana(get_mana());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    Fighter Fighter::duplicate(Fighter p_Handle,
                               Low::Core::Entity p_Entity)
    {
      return p_Handle.duplicate(p_Entity);
    }

    Low::Util::Handle Fighter::_duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Handle p_Entity)
    {
      Fighter l_Fighter = p_Handle.get_id();
      Low::Core::Entity l_Entity = p_Entity.get_id();
      return l_Fighter.duplicate(l_Entity);
    }

    void Fighter::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["mana"] = get_mana();
      p_Node["unique_id"] = get_unique_id();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Fighter::serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node)
    {
      Fighter l_Fighter = p_Handle.get_id();
      l_Fighter.serialize(p_Node);
    }

    Low::Util::Handle
    Fighter::deserialize(Low::Util::Yaml::Node &p_Node,
                         Low::Util::Handle p_Creator)
    {
      Fighter l_Handle = Fighter::make(p_Creator.get_id());

      if (p_Node["unique_id"]) {
        Low::Util::remove_unique_id(l_Handle.get_unique_id());
        l_Handle.set_unique_id(p_Node["unique_id"].as<uint64_t>());
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());
      }

      if (p_Node["deck"]) {
      }
      if (p_Node["mana"]) {
        l_Handle.set_mana(p_Node["mana"].as<uint32_t>());
      }
      if (p_Node["unique_id"]) {
        l_Handle.set_unique_id(
            p_Node["unique_id"].as<Low::Util::UniqueId>());
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    Low::Util::List<Mtd::Ability> &Fighter::get_deck() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_deck
      // LOW_CODEGEN::END::CUSTOM:GETTER_deck

      return TYPE_SOA(Fighter, deck, Low::Util::List<Mtd::Ability>);
    }
    void Fighter::set_deck(Low::Util::List<Mtd::Ability> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_deck
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_deck

      // Set new value
      TYPE_SOA(Fighter, deck, Low::Util::List<Mtd::Ability>) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_deck
      // LOW_CODEGEN::END::CUSTOM:SETTER_deck
    }

    uint32_t Fighter::get_mana() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mana
      // LOW_CODEGEN::END::CUSTOM:GETTER_mana

      return TYPE_SOA(Fighter, mana, uint32_t);
    }
    void Fighter::set_mana(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mana
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_mana

      // Set new value
      TYPE_SOA(Fighter, mana, uint32_t) = p_Value;
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
                TYPE_ID, N(mana),
                !l_Prefab.compare_property(*this, N(mana)));
          }
        }
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mana
      // LOW_CODEGEN::END::CUSTOM:SETTER_mana
    }

    Low::Core::Entity Fighter::get_entity() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity
      // LOW_CODEGEN::END::CUSTOM:GETTER_entity

      return TYPE_SOA(Fighter, entity, Low::Core::Entity);
    }
    void Fighter::set_entity(Low::Core::Entity p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

      // Set new value
      TYPE_SOA(Fighter, entity, Low::Core::Entity) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity
      // LOW_CODEGEN::END::CUSTOM:SETTER_entity
    }

    Low::Util::UniqueId Fighter::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(Fighter, unique_id, Low::Util::UniqueId);
    }
    void Fighter::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(Fighter, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
    }

    Mtd::Ability Fighter::draw()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_draw
      _LOW_ASSERT(is_alive());

      u32 l_Size = get_deck().size();

      Ability l_DrawnAbility = get_deck().front();
      // Remove first element from deck
      get_deck().erase(get_deck().begin());

      // Add ability back to the end of the deck
      get_deck().push_back(l_DrawnAbility);

      // If this fails then something went wrong when removing/adding
      // the ability
      _LOW_ASSERT(l_Size == get_deck().size());
      return l_DrawnAbility;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_draw
    }

    uint32_t Fighter::create_instance()
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

    void Fighter::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(FighterData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(FighterData, deck) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Low::Util::List<Mtd::Ability>))])
              Low::Util::List<Mtd::Ability>();
          *i_ValPtr = it->get_deck();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(FighterData, mana) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FighterData, mana) * (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(FighterData, entity) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(FighterData, entity) * (l_Capacity)],
            l_Capacity * sizeof(Low::Core::Entity));
      }
      {
        memcpy(&l_NewBuffer[offsetof(FighterData, unique_id) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FighterData, unique_id) *
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

      LOW_LOG_DEBUG << "Auto-increased budget for Fighter from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Component
} // namespace Mtd
