#include "LowCoreGameplaySystemInstance.h"

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
#include "LowCoreGameplaySystem.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    u16 GameplaySystemInstance::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        GameplaySystemInstance::IDENTIFIER(LOW_NAME(1181529166),
                                           LOW_NAME(3224277853));
    uint32_t GameplaySystemInstance::ms_Capacity = 0u;
    uint32_t GameplaySystemInstance::ms_PageSize = 0u;
    Low::Util::List<GameplaySystemInstance>
        GameplaySystemInstance::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        GameplaySystemInstance::ms_Pages;

    Low::Util::Handle
    GameplaySystemInstance::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    GameplaySystemInstance
    GameplaySystemInstance::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      GameplaySystemInstance l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = GameplaySystemInstance::ms_TypeId;

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, GameplaySystemInstance,
                                 value, GameplaySystemInstanceValue))
          GameplaySystemInstanceValue();
      ACCESSOR_TYPE_SOA(l_Handle, GameplaySystemInstance, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void GameplaySystemInstance::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        GameplaySystem l_System = get_gameplay_system();
        if (l_System.is_alive() && l_System.is_script()) {
          Scripting::ClassInstance l_ClassInstance =
              get_value().script.instance;
          if (l_ClassInstance.is_alive()) {
            l_ClassInstance.destroy();
          }
        }
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

    void GameplaySystemInstance::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowCore), N(GameplaySystemInstance));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowCore), N(GameplaySystemInstance));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, GameplaySystemInstance::Data::get_size(),
              ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(GameplaySystemInstance);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &GameplaySystemInstance::is_alive;
      l_TypeInfo.destroy = &GameplaySystemInstance::destroy;
      l_TypeInfo.serialize = &GameplaySystemInstance::serialize;
      l_TypeInfo.deserialize = &GameplaySystemInstance::deserialize;
      l_TypeInfo.find_by_index =
          &GameplaySystemInstance::_find_by_index;
      l_TypeInfo.notify = &GameplaySystemInstance::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name =
          &GameplaySystemInstance::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &GameplaySystemInstance::_make;
      l_TypeInfo.duplicate_default =
          &GameplaySystemInstance::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &GameplaySystemInstance::living_instances);
      l_TypeInfo.get_living_count =
          &GameplaySystemInstance::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: value
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(value);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GameplaySystemInstance::Data, value);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GameplaySystemInstance l_Handle = p_Handle.get_id();
          l_Handle.get_value();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GameplaySystemInstance, value,
              GameplaySystemInstanceValue);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GameplaySystemInstance l_Handle = p_Handle.get_id();
          l_Handle.set_value(*(GameplaySystemInstanceValue *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GameplaySystemInstance l_Handle = p_Handle.get_id();
          *((GameplaySystemInstanceValue *)p_Data) =
              l_Handle.get_value();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: value
      }
      {
        // Property: gameplay_system
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(gameplay_system);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GameplaySystemInstance::Data, gameplay_system);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GameplaySystemInstance l_Handle = p_Handle.get_id();
          l_Handle.get_gameplay_system();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GameplaySystemInstance, gameplay_system,
              uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GameplaySystemInstance l_Handle = p_Handle.get_id();
          l_Handle.set_gameplay_system(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GameplaySystemInstance l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_gameplay_system();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: gameplay_system
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GameplaySystemInstance::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GameplaySystemInstance l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle,
                                            GameplaySystemInstance,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GameplaySystemInstance l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GameplaySystemInstance l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: begin_play
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(begin_play);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: begin_play
      }
      {
        // Function: tick
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(tick);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Delta);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: tick
      }
      ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                        l_TypeInfo);
      // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
    }

    void GameplaySystemInstance::cleanup()
    {
      Low::Util::List<GameplaySystemInstance> l_Instances =
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
    GameplaySystemInstance::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    GameplaySystemInstance
    GameplaySystemInstance::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      GameplaySystemInstance l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = GameplaySystemInstance::ms_TypeId;

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

    GameplaySystemInstance
    GameplaySystemInstance::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      GameplaySystemInstance l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = GameplaySystemInstance::ms_TypeId;

      return l_Handle;
    }

    bool GameplaySystemInstance::is_alive() const
    {
      if (m_Data.m_Type != GameplaySystemInstance::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == GameplaySystemInstance::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t GameplaySystemInstance::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    GameplaySystemInstance::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    GameplaySystemInstance
    GameplaySystemInstance::find_by_name(Low::Util::Name p_Name)
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

    GameplaySystemInstance
    GameplaySystemInstance::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      LOW_ASSERT_WARN(false, "Not implemented");
      return 0;
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE
    }

    GameplaySystemInstance
    GameplaySystemInstance::duplicate(GameplaySystemInstance p_Handle,
                                      Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    GameplaySystemInstance::_duplicate(Low::Util::Handle p_Handle,
                                       Low::Util::Name p_Name)
    {
      GameplaySystemInstance l_GameplaySystemInstance =
          p_Handle.get_id();
      return l_GameplaySystemInstance.duplicate(p_Name);
    }

    void GameplaySystemInstance::serialize(
        Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void
    GameplaySystemInstance::serialize(Low::Util::Handle p_Handle,
                                      Low::Util::Serial::Node &p_Node)
    {
      GameplaySystemInstance l_GameplaySystemInstance =
          p_Handle.get_id();
      l_GameplaySystemInstance.serialize(p_Node);
    }

    Low::Util::Handle GameplaySystemInstance::deserialize(
        Low::Util::Serial::Node &p_Node, Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      LOW_NOT_IMPLEMENTED_WARN;
      return DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void GameplaySystemInstance::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 GameplaySystemInstance::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 GameplaySystemInstance::observe(
        Low::Util::Name p_Observable,
        Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void GameplaySystemInstance::notify(Low::Util::Handle p_Observed,
                                        Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void GameplaySystemInstance::_notify(Low::Util::Handle p_Observer,
                                         Low::Util::Handle p_Observed,
                                         Low::Util::Name p_Observable)
    {
      GameplaySystemInstance l_GameplaySystemInstance =
          p_Observer.get_id();
      l_GameplaySystemInstance.notify(p_Observed, p_Observable);
    }

    GameplaySystemInstanceValue &
    GameplaySystemInstance::get_value() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_value
      // LOW_CODEGEN::END::CUSTOM:GETTER_value

      return TYPE_SOA(GameplaySystemInstance, value,
                      GameplaySystemInstanceValue);
    }
    void GameplaySystemInstance::set_value(
        GameplaySystemInstanceValue &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_value
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_value

      // Set new value
      TYPE_SOA(GameplaySystemInstance, value,
               GameplaySystemInstanceValue) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_value
      // LOW_CODEGEN::END::CUSTOM:SETTER_value

      broadcast_observable(N(value));
    }

    uint64_t GameplaySystemInstance::get_gameplay_system() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gameplay_system
      // LOW_CODEGEN::END::CUSTOM:GETTER_gameplay_system

      return TYPE_SOA(GameplaySystemInstance, gameplay_system,
                      uint64_t);
    }
    void GameplaySystemInstance::set_gameplay_system(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gameplay_system
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gameplay_system

      // Set new value
      TYPE_SOA(GameplaySystemInstance, gameplay_system, uint64_t) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gameplay_system
      // LOW_CODEGEN::END::CUSTOM:SETTER_gameplay_system

      broadcast_observable(N(gameplay_system));
    }

    Low::Util::Name GameplaySystemInstance::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(GameplaySystemInstance, name, Low::Util::Name);
    }
    void GameplaySystemInstance::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(GameplaySystemInstance, name, Low::Util::Name) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    void GameplaySystemInstance::begin_play()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_begin_play
      GameplaySystem l_System = get_gameplay_system();
      if (l_System.is_script()) {
        Scripting::ClassInstance l_ClassInstance =
            get_value().script.instance;

        l_ClassInstance.call_method("void begin_play()");
        return;
      }

      LOW_LOG_WARN
          << "Gameplay system does not have valid begin_play method."
          << LOW_LOG_END;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_begin_play
    }

    void GameplaySystemInstance::tick(float p_Delta)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_tick
      GameplaySystem l_System = get_gameplay_system();
      if (l_System.is_script()) {
        Scripting::ClassInstance l_ClassInstance =
            get_value().script.instance;

        l_ClassInstance.call_method("void tick(float)", p_Delta);
        return;
      }

      LOW_LOG_WARN
          << "Gameplay system does not have valid tick method."
          << LOW_LOG_END;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_tick
    }

    uint32_t GameplaySystemInstance::create_instance(u32 &p_PageIndex,
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

    u32 GameplaySystemInstance::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT(
          (l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
          "Could not increase capacity for GameplaySystemInstance.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, GameplaySystemInstance::Data::get_size(),
          ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool GameplaySystemInstance::get_page_for_index(const u32 p_Index,
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
