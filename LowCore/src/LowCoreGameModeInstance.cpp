#include "LowCoreGameModeInstance.h"

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
#include "LowCoreScriptClassInstance.h"
#include "LowUtilVariant.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    u16 GameModeInstance::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        GameModeInstance::IDENTIFIER(LOW_NAME(1181529166),
                                     LOW_NAME(2441016590));
    uint32_t GameModeInstance::ms_Capacity = 0u;
    uint32_t GameModeInstance::ms_PageSize = 0u;
    Low::Util::List<GameModeInstance>
        GameModeInstance::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        GameModeInstance::ms_Pages;

    Low::Util::Handle GameModeInstance::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    GameModeInstance GameModeInstance::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      GameModeInstance l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = GameModeInstance::ms_TypeId;

      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, GameModeInstance, systems,
          Low::Util::List<GameplaySystemInstance>))
          Low::Util::List<GameplaySystemInstance>();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, GameModeInstance, gamemode,
                                 Low::Core::GameMode))
          Low::Core::GameMode();
      ACCESSOR_TYPE_SOA(l_Handle, GameModeInstance, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void GameModeInstance::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        for (GameplaySystemInstance i_System : get_systems()) {
          i_System.destroy();
        }
        get_systems().clear();
        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

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

    void GameModeInstance::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowCore), N(GameModeInstance));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowCore), N(GameModeInstance));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, GameModeInstance::Data::get_size(),
              ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(GameModeInstance);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &GameModeInstance::is_alive;
      l_TypeInfo.destroy = &GameModeInstance::destroy;
      l_TypeInfo.serialize = &GameModeInstance::serialize;
      l_TypeInfo.deserialize = &GameModeInstance::deserialize;
      l_TypeInfo.find_by_index = &GameModeInstance::_find_by_index;
      l_TypeInfo.notify = &GameModeInstance::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &GameModeInstance::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &GameModeInstance::_make;
      l_TypeInfo.duplicate_default = &GameModeInstance::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &GameModeInstance::living_instances);
      l_TypeInfo.get_living_count = &GameModeInstance::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: systems
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(systems);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GameModeInstance::Data, systems);
        l_PropertyInfo.size = sizeof(GameModeInstance::Data::systems);
        l_PropertyInfo.containerType =
            Low::Util::RTTI::ContainerType::LIST;
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GameModeInstance l_Handle = p_Handle.get_id();
          l_Handle.get_systems();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GameModeInstance, systems,
              Low::Util::List<GameplaySystemInstance>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GameModeInstance l_Handle = p_Handle.get_id();
          *((void **)p_Data) = &l_Handle.get_systems();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: systems
      }
      {
        // Property: gamemode
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(gamemode);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GameModeInstance::Data, gamemode);
        l_PropertyInfo.size =
            sizeof(GameModeInstance::Data::gamemode);
        l_PropertyInfo.containerType =
            Low::Util::RTTI::ContainerType::NONE;
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GameModeInstance l_Handle = p_Handle.get_id();
          l_Handle.get_gamemode();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GameModeInstance, gamemode,
              Low::Core::GameMode);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GameModeInstance l_Handle = p_Handle.get_id();
          *((Low::Core::GameMode *)p_Data) = l_Handle.get_gamemode();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: gamemode
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GameModeInstance::Data, name);
        l_PropertyInfo.size = sizeof(GameModeInstance::Data::name);
        l_PropertyInfo.containerType =
            Low::Util::RTTI::ContainerType::NONE;
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GameModeInstance l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GameModeInstance, name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GameModeInstance l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GameModeInstance l_Handle = p_Handle.get_id();
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
        l_FunctionInfo.handleType = GameModeInstance::type_id();
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_GameMode);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = Low::Core::GameMode::type_id();
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: apply_values_from_gamemode
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(apply_values_from_gamemode);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: apply_values_from_gamemode
      }
      ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                        l_TypeInfo);
      // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
    }

    void GameModeInstance::cleanup()
    {
      Low::Util::List<GameModeInstance> l_Instances =
          ms_LivingInstances;
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

    Low::Util::Handle
    GameModeInstance::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    GameModeInstance GameModeInstance::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      GameModeInstance l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = GameModeInstance::ms_TypeId;

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

    GameModeInstance
    GameModeInstance::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      GameModeInstance l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = GameModeInstance::ms_TypeId;

      return l_Handle;
    }

    bool GameModeInstance::is_alive() const
    {
      if (m_Data.m_Type != GameModeInstance::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == GameModeInstance::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t GameModeInstance::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    GameModeInstance::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    GameModeInstance
    GameModeInstance::find_by_name(Low::Util::Name p_Name)
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

    GameModeInstance
    GameModeInstance::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      LOW_ASSERT_WARN(false, "Not implemented");
      return 0;
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE
    }

    GameModeInstance
    GameModeInstance::duplicate(GameModeInstance p_Handle,
                                Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    GameModeInstance::_duplicate(Low::Util::Handle p_Handle,
                                 Low::Util::Name p_Name)
    {
      GameModeInstance l_GameModeInstance = p_Handle.get_id();
      return l_GameModeInstance.duplicate(p_Name);
    }

    void
    GameModeInstance::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void GameModeInstance::serialize(Low::Util::Handle p_Handle,
                                     Low::Util::Serial::Node &p_Node)
    {
      GameModeInstance l_GameModeInstance = p_Handle.get_id();
      l_GameModeInstance.serialize(p_Node);
    }

    Low::Util::Handle
    GameModeInstance::deserialize(Low::Util::Serial::Node &p_Node,
                                  Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void GameModeInstance::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 GameModeInstance::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 GameModeInstance::observe(Low::Util::Name p_Observable,
                                  Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void GameModeInstance::notify(Low::Util::Handle p_Observed,
                                  Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void GameModeInstance::_notify(Low::Util::Handle p_Observer,
                                   Low::Util::Handle p_Observed,
                                   Low::Util::Name p_Observable)
    {
      GameModeInstance l_GameModeInstance = p_Observer.get_id();
      l_GameModeInstance.notify(p_Observed, p_Observable);
    }

    Low::Util::List<GameplaySystemInstance> &
    GameModeInstance::get_systems() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_systems
      // LOW_CODEGEN::END::CUSTOM:GETTER_systems

      return TYPE_SOA(GameModeInstance, systems,
                      Low::Util::List<GameplaySystemInstance>);
    }

    Low::Core::GameMode &GameModeInstance::get_gamemode() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gamemode
      // LOW_CODEGEN::END::CUSTOM:GETTER_gamemode

      return TYPE_SOA(GameModeInstance, gamemode,
                      Low::Core::GameMode);
    }
    void GameModeInstance::set_gamemode(Low::Core::GameMode &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gamemode
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gamemode

      // Set new value
      TYPE_SOA(GameModeInstance, gamemode, Low::Core::GameMode) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gamemode
      // LOW_CODEGEN::END::CUSTOM:SETTER_gamemode

      broadcast_observable(N(gamemode));
    }

    Low::Util::Name GameModeInstance::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(GameModeInstance, name, Low::Util::Name);
    }
    void GameModeInstance::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(GameModeInstance, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    GameModeInstance
    GameModeInstance::make(Low::Core::GameMode p_GameMode)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      GameModeInstance l_Instance = make(p_GameMode.get_name());

      l_Instance.set_gamemode(p_GameMode);

      for (GameModeSystemStorage &i_Storage :
           p_GameMode.get_systems()) {
        GameplaySystemInstance i_Instance =
            i_Storage.system.spawn_instance();

        l_Instance.get_systems().push_back(i_Instance);
      }

      l_Instance.apply_values_from_gamemode();

      return l_Instance;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    void GameModeInstance::apply_values_from_gamemode()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_apply_values_from_gamemode
      for (const GameModeSystemStorage &i_Storage :
           get_gamemode().get_systems()) {
        for (GameplaySystemInstance &i_System : get_systems()) {
          GameplaySystem i_GameplaySystem =
              i_System.get_gameplay_system();

          if (i_GameplaySystem == i_Storage.system) {
            for (const auto &[i_Name, i_Val] : i_Storage.values) {
              if (i_GameplaySystem.is_script()) {
                Scripting::ClassInstance i_ClassInstance =
                    i_System.get_value().script.instance;

                if (i_Val.m_Type == Util::VariantType::String) {
                  const Util::String i_Str = i_Val.as_string();

                  i_ClassInstance.set_member(i_Name, &i_Str,
                                             sizeof(Util::String));
                } else {
                  i_ClassInstance.set_member(
                      i_Name, &i_Val.m_Bool,
                      Util::variant_type_size(
                          (Util::VariantType::Enum)i_Val.m_Type));
                }
              }
            }
            break;
          }
        }
      }

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_apply_values_from_gamemode
    }

    uint32_t GameModeInstance::create_instance(u32 &p_PageIndex,
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

    u32 GameModeInstance::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for GameModeInstance.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, GameModeInstance::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool GameModeInstance::get_page_for_index(const u32 p_Index,
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
