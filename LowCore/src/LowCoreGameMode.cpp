#include "LowCoreGameMode.h"

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
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    u16 GameMode::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        GameMode::IDENTIFIER(LOW_NAME(1181529166),
                             LOW_NAME(2562687228));
    uint32_t GameMode::ms_Capacity = 0u;
    uint32_t GameMode::ms_PageSize = 0u;
    Low::Util::List<GameMode> GameMode::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *> GameMode::ms_Pages;

    Low::Util::Handle GameMode::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    GameMode GameMode::make(Low::Util::Name p_Name)
    {
      return make(p_Name, 0ull);
    }

    GameMode GameMode::make(Low::Util::Name p_Name,
                            Low::Util::UniqueId p_UniqueId)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      GameMode l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = GameMode::ms_TypeId;

      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, GameMode, systems,
          Low::Util::List<GameModeSystemStorage>))
          Low::Util::List<GameModeSystemStorage>();
      ACCESSOR_TYPE_SOA(l_Handle, GameMode, blocks_underlying, bool) =
          false;
      ACCESSOR_TYPE_SOA(l_Handle, GameMode, name, Low::Util::Name) =
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
      l_Handle.blocks_underlying(true);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void GameMode::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
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

      l_Page->slots[l_SlotIndex].m_Occupied = false;
      l_Page->slots[l_SlotIndex].m_Generation++;

      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end();) {
        if (it->get_id() == get_id()) {
          it = ms_LivingInstances.erase(it);
        } else {
          it++;
        }
      }
    }

    void GameMode::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(N(LowCore),
                                                        N(GameMode));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowCore), N(GameMode));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, GameMode::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(GameMode);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &GameMode::is_alive;
      l_TypeInfo.destroy = &GameMode::destroy;
      l_TypeInfo.serialize = &GameMode::serialize;
      l_TypeInfo.deserialize = &GameMode::deserialize;
      l_TypeInfo.find_by_index = &GameMode::_find_by_index;
      l_TypeInfo.notify = &GameMode::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &GameMode::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &GameMode::_make;
      l_TypeInfo.duplicate_default = &GameMode::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &GameMode::living_instances);
      l_TypeInfo.get_living_count = &GameMode::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: systems
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(systems);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(GameMode::Data, systems);
        l_PropertyInfo.size = sizeof(GameMode::Data::systems);
        l_PropertyInfo.containerType =
            Low::Util::RTTI::ContainerType::LIST;
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GameMode l_Handle = p_Handle.get_id();
          l_Handle.get_systems();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GameMode, systems,
              Low::Util::List<GameModeSystemStorage>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GameMode l_Handle = p_Handle.get_id();
          *((void **)p_Data) = &l_Handle.get_systems();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: systems
      }
      {
        // Property: blocks_underlying
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(blocks_underlying);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GameMode::Data, blocks_underlying);
        l_PropertyInfo.size =
            sizeof(GameMode::Data::blocks_underlying);
        l_PropertyInfo.containerType =
            Low::Util::RTTI::ContainerType::NONE;
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GameMode l_Handle = p_Handle.get_id();
          l_Handle.blocks_underlying();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GameMode,
                                            blocks_underlying, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GameMode l_Handle = p_Handle.get_id();
          l_Handle.blocks_underlying(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GameMode l_Handle = p_Handle.get_id();
          *((bool *)p_Data) = l_Handle.blocks_underlying();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: blocks_underlying
      }
      {
        // Property: unique_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(unique_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GameMode::Data, unique_id);
        l_PropertyInfo.size = sizeof(GameMode::Data::unique_id);
        l_PropertyInfo.containerType =
            Low::Util::RTTI::ContainerType::NONE;
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GameMode l_Handle = p_Handle.get_id();
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GameMode, unique_id, Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GameMode l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(GameMode::Data, name);
        l_PropertyInfo.size = sizeof(GameMode::Data::name);
        l_PropertyInfo.containerType =
            Low::Util::RTTI::ContainerType::NONE;
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GameMode l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GameMode, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GameMode l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GameMode l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                        l_TypeInfo);
      // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
    }

    void GameMode::cleanup()
    {
      Low::Util::List<GameMode> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
        Low::Util::Instances::Page *i_Page = *it;
        free(i_Page->buffer);
        free(i_Page->slots);
        delete i_Page;
        it = ms_Pages.erase(it);
      }

      ms_Capacity = 0;
    }

    Low::Util::Handle GameMode::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    GameMode GameMode::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      GameMode l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = GameMode::ms_TypeId;

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
        l_Handle.m_Data.m_Generation = 0;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      l_Handle.m_Data.m_Generation =
          l_Page->slots[l_SlotIndex].m_Generation;

      return l_Handle;
    }

    GameMode GameMode::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      GameMode l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = GameMode::ms_TypeId;

      return l_Handle;
    }

    bool GameMode::is_alive() const
    {
      if (m_Data.m_Type != GameMode::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == GameMode::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t GameMode::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle GameMode::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    GameMode GameMode::find_by_name(Low::Util::Name p_Name)
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

    GameMode GameMode::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      GameMode l_Handle = make(p_Name);
      l_Handle.blocks_underlying(blocks_underlying());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    GameMode GameMode::duplicate(GameMode p_Handle,
                                 Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle GameMode::_duplicate(Low::Util::Handle p_Handle,
                                           Low::Util::Name p_Name)
    {
      GameMode l_GameMode = p_Handle.get_id();
      return l_GameMode.duplicate(p_Name);
    }

    void GameMode::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["blocks_underlying"] = blocks_underlying();
      p_Node["_unique_id"] = Low::Util::U64Id{get_unique_id()};
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      for (const GameModeSystemStorage &i_Storage : get_systems()) {
        if (!i_Storage.system.is_alive()) {
          continue;
        }

        Util::Serial::Node i_StorageNode;

        i_StorageNode["system_name"] = i_Storage.system.get_name();

        for (auto &it : i_Storage.values) {
          Util::Serial::serialize_variant(
              i_StorageNode["values"][it.first.c_str()], it.second);
        }

        p_Node["systems"].push_back(i_StorageNode);
      }
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void GameMode::serialize(Low::Util::Handle p_Handle,
                             Low::Util::Serial::Node &p_Node)
    {
      GameMode l_GameMode = p_Handle.get_id();
      l_GameMode.serialize(p_Node);
    }

    Low::Util::Handle
    GameMode::deserialize(Low::Util::Serial::Node &p_Node,
                          Low::Util::Handle p_Creator)
    {
      Low::Util::UniqueId l_HandleUniqueId = 0ull;
      if (p_Node["unique_id"]) {
        l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
      } else if (p_Node["_unique_id"]) {
        l_HandleUniqueId = Low::Util::string_to_hash(
            p_Node["_unique_id"].as<Low::Util::String>());
      }

      GameMode l_Handle =
          GameMode::make(N(GameMode), l_HandleUniqueId);

      if (p_Node["blocks_underlying"]) {
        l_Handle.blocks_underlying(
            p_Node["blocks_underlying"].as<bool>());
      }
      if (p_Node["unique_id"]) {
        l_Handle.set_unique_id(
            p_Node["unique_id"].as<Low::Util::UniqueId>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(p_Node["name"].as<Low::Util::Name>());
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      if (p_Node["systems"]) {
        for (int i = 0; i < p_Node["systems"].size(); ++i) {
          Util::Serial::Node &i_StorageNode = p_Node["systems"][i];
          if (!i_StorageNode["system_name"]) {
            continue;
          }

          GameModeSystemStorage i_Storage;
          const Util::Name i_SystemName =
              i_StorageNode["system_name"].as<Util::Name>();
          i_Storage.system =
              GameplaySystem::find_by_name(i_SystemName);

          if (!i_Storage.system.is_alive()) {
            LOW_LOG_WARN
                << "Failed to find system by name: " << i_SystemName
                << LOW_LOG_END;
            continue;
          }

          if (i_StorageNode["values"]) {
            for (auto [key, value] : i_StorageNode["values"]) {
              i_Storage.values[LOW_NAME(key->c_str())] =
                  Util::Serial::deserialize_variant(value);
            }
          }
          l_Handle.get_systems().push_back(i_Storage);
        }
      }
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void
    GameMode::broadcast_observable(Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 GameMode::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 GameMode::observe(Low::Util::Name p_Observable,
                          Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void GameMode::notify(Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY

      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void GameMode::_notify(Low::Util::Handle p_Observer,
                           Low::Util::Handle p_Observed,
                           Low::Util::Name p_Observable)
    {
      GameMode l_GameMode = p_Observer.get_id();
      l_GameMode.notify(p_Observed, p_Observable);
    }

    Low::Util::List<GameModeSystemStorage> &
    GameMode::get_systems() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_systems
      // LOW_CODEGEN::END::CUSTOM:GETTER_systems

      return TYPE_SOA(GameMode, systems,
                      Low::Util::List<GameModeSystemStorage>);
    }

    bool GameMode::blocks_underlying() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_blocks_underlying
      // LOW_CODEGEN::END::CUSTOM:GETTER_blocks_underlying

      return TYPE_SOA(GameMode, blocks_underlying, bool);
    }
    void GameMode::toggle_blocks_underlying()
    {
      blocks_underlying(!blocks_underlying());
    }

    void GameMode::blocks_underlying(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_blocks_underlying
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_blocks_underlying

      // Set new value
      TYPE_SOA(GameMode, blocks_underlying, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_blocks_underlying
      // LOW_CODEGEN::END::CUSTOM:SETTER_blocks_underlying

      broadcast_observable(N(blocks_underlying));
    }

    Low::Util::UniqueId GameMode::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(GameMode, unique_id, Low::Util::UniqueId);
    }
    void GameMode::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(GameMode, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

      broadcast_observable(N(unique_id));
    }

    Low::Util::Name GameMode::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(GameMode, name, Low::Util::Name);
    }
    void GameMode::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(GameMode, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t GameMode::create_instance(u32 &p_PageIndex,
                                       u32 &p_SlotIndex)
    {
      u32 l_Index = 0;
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      bool l_FoundIndex = false;

      for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
           ++l_PageIndex) {
        for (l_SlotIndex = 0;
             l_SlotIndex < ms_Pages[l_PageIndex]->size;
             ++l_SlotIndex) {
          if (!ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied) {
            l_FoundIndex = true;
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
      }
      ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
      p_PageIndex = l_PageIndex;
      p_SlotIndex = l_SlotIndex;
      return l_Index;
    }

    u32 GameMode::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for GameMode.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, GameMode::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool GameMode::get_page_for_index(const u32 p_Index,
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
