#include "LowCoreGameplaySystem.h"

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

    u16 GameplaySystem::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        GameplaySystem::IDENTIFIER(LOW_NAME(1181529166),
                                   LOW_NAME(3881305946));
    uint32_t GameplaySystem::ms_Capacity = 0u;
    uint32_t GameplaySystem::ms_PageSize = 0u;
    Low::Util::List<GameplaySystem>
        GameplaySystem::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        GameplaySystem::ms_Pages;

    Low::Util::Handle GameplaySystem::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    GameplaySystem GameplaySystem::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      GameplaySystem l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = GameplaySystem::ms_TypeId;

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, GameplaySystem, value,
                                 GameplaySystemValue))
          GameplaySystemValue();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, GameplaySystem, type,
                                 GameplaySystemType))
          GameplaySystemType();
      ACCESSOR_TYPE_SOA(l_Handle, GameplaySystem, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void GameplaySystem::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
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

    void GameplaySystem::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowCore), N(GameplaySystem));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowCore), N(GameplaySystem));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, GameplaySystem::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(GameplaySystem);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &GameplaySystem::is_alive;
      l_TypeInfo.destroy = &GameplaySystem::destroy;
      l_TypeInfo.serialize = &GameplaySystem::serialize;
      l_TypeInfo.deserialize = &GameplaySystem::deserialize;
      l_TypeInfo.find_by_index = &GameplaySystem::_find_by_index;
      l_TypeInfo.notify = &GameplaySystem::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &GameplaySystem::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &GameplaySystem::_make;
      l_TypeInfo.duplicate_default = &GameplaySystem::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &GameplaySystem::living_instances);
      l_TypeInfo.get_living_count = &GameplaySystem::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: value
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(value);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GameplaySystem::Data, value);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GameplaySystem l_Handle = p_Handle.get_id();
          l_Handle.get_value();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GameplaySystem, value, GameplaySystemValue);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GameplaySystem l_Handle = p_Handle.get_id();
          l_Handle.set_value(*(GameplaySystemValue *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GameplaySystem l_Handle = p_Handle.get_id();
          *((GameplaySystemValue *)p_Data) = l_Handle.get_value();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: value
      }
      {
        // Property: type
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(type);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GameplaySystem::Data, type);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GameplaySystem l_Handle = p_Handle.get_id();
          l_Handle.get_type();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GameplaySystem,
                                            type, GameplaySystemType);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GameplaySystem l_Handle = p_Handle.get_id();
          *((GameplaySystemType *)p_Data) = l_Handle.get_type();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: type
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GameplaySystem::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GameplaySystem l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GameplaySystem,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GameplaySystem l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GameplaySystem l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: make_code
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make_code);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType =
            Low::Core::GameplaySystem::type_id();
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make_code
      }
      {
        // Function: make_script
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make_script);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType =
            Low::Core::GameplaySystem::type_id();
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Class);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Core::Scripting::Class::type_id();
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make_script
      }
      {
        // Function: spawn_instance
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(spawn_instance);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType =
            Low::Core::GameplaySystemInstance::type_id();
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: spawn_instance
      }
      {
        // Function: find_by_scriptclass
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(find_by_scriptclass);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType =
            Low::Core::GameplaySystem::type_id();
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_ScriptClass);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Core::Scripting::Class::type_id();
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: find_by_scriptclass
      }
      {
        // Function: is_script
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(is_script);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: is_script
      }
      ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                        l_TypeInfo);
      // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
    }

    void GameplaySystem::cleanup()
    {
      Low::Util::List<GameplaySystem> l_Instances =
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

    Low::Util::Handle GameplaySystem::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    GameplaySystem GameplaySystem::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      GameplaySystem l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = GameplaySystem::ms_TypeId;

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

    GameplaySystem GameplaySystem::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      GameplaySystem l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = GameplaySystem::ms_TypeId;

      return l_Handle;
    }

    bool GameplaySystem::is_alive() const
    {
      if (m_Data.m_Type != GameplaySystem::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == GameplaySystem::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t GameplaySystem::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    GameplaySystem::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    GameplaySystem
    GameplaySystem::find_by_name(Low::Util::Name p_Name)
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

    GameplaySystem
    GameplaySystem::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      LOW_ASSERT_WARN(false, "Not implemented");
      return 0;
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE
    }

    GameplaySystem GameplaySystem::duplicate(GameplaySystem p_Handle,
                                             Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    GameplaySystem::_duplicate(Low::Util::Handle p_Handle,
                               Low::Util::Name p_Name)
    {
      GameplaySystem l_GameplaySystem = p_Handle.get_id();
      return l_GameplaySystem.duplicate(p_Name);
    }

    void
    GameplaySystem::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void GameplaySystem::serialize(Low::Util::Handle p_Handle,
                                   Low::Util::Serial::Node &p_Node)
    {
      GameplaySystem l_GameplaySystem = p_Handle.get_id();
      l_GameplaySystem.serialize(p_Node);
    }

    Low::Util::Handle
    GameplaySystem::deserialize(Low::Util::Serial::Node &p_Node,
                                Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      LOW_NOT_IMPLEMENTED_WARN;
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void GameplaySystem::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 GameplaySystem::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 GameplaySystem::observe(Low::Util::Name p_Observable,
                                Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void GameplaySystem::notify(Low::Util::Handle p_Observed,
                                Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void GameplaySystem::_notify(Low::Util::Handle p_Observer,
                                 Low::Util::Handle p_Observed,
                                 Low::Util::Name p_Observable)
    {
      GameplaySystem l_GameplaySystem = p_Observer.get_id();
      l_GameplaySystem.notify(p_Observed, p_Observable);
    }

    GameplaySystemValue &GameplaySystem::get_value() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_value
      // LOW_CODEGEN::END::CUSTOM:GETTER_value

      return TYPE_SOA(GameplaySystem, value, GameplaySystemValue);
    }
    void GameplaySystem::set_value(GameplaySystemValue &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_value
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_value

      // Set new value
      TYPE_SOA(GameplaySystem, value, GameplaySystemValue) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_value
      // LOW_CODEGEN::END::CUSTOM:SETTER_value

      broadcast_observable(N(value));
    }

    GameplaySystemType GameplaySystem::get_type() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_type
      // LOW_CODEGEN::END::CUSTOM:GETTER_type

      return TYPE_SOA(GameplaySystem, type, GameplaySystemType);
    }
    void GameplaySystem::set_type(GameplaySystemType p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_type
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_type

      // Set new value
      TYPE_SOA(GameplaySystem, type, GameplaySystemType) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_type
      // LOW_CODEGEN::END::CUSTOM:SETTER_type

      broadcast_observable(N(type));
    }

    Low::Util::Name GameplaySystem::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(GameplaySystem, name, Low::Util::Name);
    }
    void GameplaySystem::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(GameplaySystem, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    Low::Core::GameplaySystem
    GameplaySystem::make_code(Low::Util::Name p_Name)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_code
      GameplaySystem l_System = make(p_Name);
      l_System.set_type(GameplaySystemType::Code);

      // TODO: Set value

      return l_System;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_code
    }

    Low::Core::GameplaySystem
    GameplaySystem::make_script(Low::Util::Name p_Name,
                                Low::Core::Scripting::Class p_Class)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_script
      GameplaySystem l_System = make(p_Name);
      l_System.set_type(GameplaySystemType::Script);

      l_System.get_value().script.sclass = p_Class;

      return l_System;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_script
    }

    Low::Core::GameplaySystemInstance GameplaySystem::spawn_instance()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_spawn_instance
      GameplaySystemInstance l_Instance =
          GameplaySystemInstance::make(get_name());
      l_Instance.set_gameplay_system(get_id());

      if (is_script()) {
        l_Instance.get_value().script.instance =
            get_value().script.sclass.spawn_instance(get_name());
      } else {
        LOW_ASSERT(false, "Unsupported gameplay system type.");
      }

      return l_Instance;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_spawn_instance
    }

    Low::Core::GameplaySystem GameplaySystem::find_by_scriptclass(
        Low::Core::Scripting::Class p_ScriptClass)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_find_by_scriptclass
      for (u32 i = 0; i < living_count(); ++i) {
        GameplaySystem i_GameplaySystem = living_instances()[i];

        if (i_GameplaySystem.is_script() &&
            i_GameplaySystem.get_value().script.sclass ==
                p_ScriptClass) {
          return i_GameplaySystem;
        }
      }
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_find_by_scriptclass
    }

    bool GameplaySystem::is_script() const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_script
      return get_type() == GameplaySystemType::Script;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_script
    }

    uint32_t GameplaySystem::create_instance(u32 &p_PageIndex,
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

    u32 GameplaySystem::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for GameplaySystem.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, GameplaySystem::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool GameplaySystem::get_page_for_index(const u32 p_Index,
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
