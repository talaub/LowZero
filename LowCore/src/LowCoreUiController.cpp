#include "LowCoreUiController.h"

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
    namespace UI {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 Controller::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          Controller::IDENTIFIER(LOW_NAME(1181529166),
                                 LOW_NAME(61826378));
      uint32_t Controller::ms_Capacity = 0u;
      uint32_t Controller::ms_PageSize = 0u;
      Low::Util::SharedMutex Controller::ms_LivingMutex;
      Low::Util::SharedMutex Controller::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          Controller::ms_PagesLock(Controller::ms_PagesMutex,
                                   std::defer_lock);
      Low::Util::List<Controller> Controller::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          Controller::ms_Pages;

      Low::Util::Handle Controller::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Controller Controller::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        Controller l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Controller::ms_TypeId;

        l_PageLock.unlock();

        Low::Util::HandleLock<Controller> l_HandleLock(l_Handle);

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Controller, value,
                                   ControllerValue))
            ControllerValue();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Controller, type,
                                   ControllerType)) ControllerType();
        ACCESSOR_TYPE_SOA(l_Handle, Controller, name,
                          Low::Util::Name) = Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        {
          Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          ms_LivingInstances.push_back(l_Handle);
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        LOW_LOG_DEBUG << "Create UI controller: '" << p_Name << "'"
                      << LOW_LOG_END;
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Controller::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<Controller> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        _LOW_ASSERT(get_page_for_index(get_index(), l_PageIndex,
                                       l_SlotIndex));
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
        l_Page->slots[l_SlotIndex].m_Occupied = false;
        l_Page->slots[l_SlotIndex].m_Generation++;

        ms_PagesLock.lock();
        Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end();) {
          if (it->get_id() == get_id()) {
            it = ms_LivingInstances.erase(it);
          } else {
            it++;
          }
        }
        ms_PagesLock.unlock();
        l_LivingLock.unlock();
      }

      void Controller::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowCore), N(Controller));

        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowCore),
                                                      N(Controller));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Controller::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Controller);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Controller::is_alive;
        l_TypeInfo.destroy = &Controller::destroy;
        l_TypeInfo.serialize = &Controller::serialize;
        l_TypeInfo.deserialize = &Controller::deserialize;
        l_TypeInfo.find_by_index = &Controller::_find_by_index;
        l_TypeInfo.notify = &Controller::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &Controller::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Controller::_make;
        l_TypeInfo.duplicate_default = &Controller::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Controller::living_instances);
        l_TypeInfo.get_living_count = &Controller::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: value
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(value);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Controller::Data, value);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Controller l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Controller> l_HandleLock(l_Handle);
            l_Handle.get_value();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Controller,
                                              value, ControllerValue);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Controller l_Handle = p_Handle.get_id();
            l_Handle.set_value(*(ControllerValue *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Controller l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Controller> l_HandleLock(l_Handle);
            *((ControllerValue *)p_Data) = l_Handle.get_value();
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
              offsetof(Controller::Data, type);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Controller l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Controller> l_HandleLock(l_Handle);
            l_Handle.get_type();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Controller,
                                              type, ControllerType);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Controller l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Controller> l_HandleLock(l_Handle);
            *((ControllerType *)p_Data) = l_Handle.get_type();
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
              offsetof(Controller::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Controller l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Controller> l_HandleLock(l_Handle);
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Controller,
                                              name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Controller l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Controller l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Controller> l_HandleLock(l_Handle);
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
              Low::Core::UI::Controller::type_id();
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Name);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::NAME;
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
              Low::Core::UI::Controller::type_id();
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Name);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::NAME;
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
              Low::Core::UI::ControllerInstance::type_id();
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: spawn_instance
        }
        {
          // Function: find_by_scriptclass
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(find_by_scriptclass);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType =
              Low::Core::UI::Controller::type_id();
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
          // Function: is_script_controller
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(is_script_controller);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: is_script_controller
        }
        {
          // Function: update_instances
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(update_instances);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: update_instances
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void Controller::cleanup()
      {
        Low::Util::List<Controller> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        ms_PagesLock.lock();
        for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
          Low::Util::Instances::Page *i_Page = *it;
          free(i_Page->buffer);
          free(i_Page->slots);
          free(i_Page->lockWords);
          delete i_Page;
          it = ms_Pages.erase(it);
        }

        ms_Capacity = 0;

        ms_PagesLock.unlock();
      }

      Low::Util::Handle Controller::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Controller Controller::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Controller l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Controller::ms_TypeId;

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
          l_Handle.m_Data.m_Generation = 0;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
        l_Handle.m_Data.m_Generation =
            l_Page->slots[l_SlotIndex].m_Generation;

        return l_Handle;
      }

      Controller Controller::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Controller l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Controller::ms_TypeId;

        return l_Handle;
      }

      bool Controller::is_alive() const
      {
        if (m_Data.m_Type != Controller::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
        return m_Data.m_Type == Controller::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Controller::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      Controller::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      Controller Controller::find_by_name(Low::Util::Name p_Name)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME
        // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

        Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
        return Low::Util::Handle::DEAD;
      }

      Controller Controller::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        LOW_ASSERT_WARN(false, "Not implemented");
        return 0;
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE
      }

      Controller Controller::duplicate(Controller p_Handle,
                                       Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      Controller::_duplicate(Low::Util::Handle p_Handle,
                             Low::Util::Name p_Name)
      {
        Controller l_Controller = p_Handle.get_id();
        return l_Controller.duplicate(p_Name);
      }

      void
      Controller::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Controller::serialize(Low::Util::Handle p_Handle,
                                 Low::Util::Serial::Node &p_Node)
      {
        Controller l_Controller = p_Handle.get_id();
        l_Controller.serialize(p_Node);
      }

      Low::Util::Handle
      Controller::deserialize(Low::Util::Serial::Node &p_Node,
                              Low::Util::Handle p_Creator)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        return Util::Handle::DEAD;
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
      }

      void Controller::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64
      Controller::observe(Low::Util::Name p_Observable,
                          Low::Util::Function<void(Low::Util::Handle,
                                                   Low::Util::Name)>
                              p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Controller::observe(Low::Util::Name p_Observable,
                              Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Controller::notify(Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Controller::_notify(Low::Util::Handle p_Observer,
                               Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
      {
        Controller l_Controller = p_Observer.get_id();
        l_Controller.notify(p_Observed, p_Observable);
      }

      ControllerValue &Controller::get_value() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Controller> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_value
        // LOW_CODEGEN::END::CUSTOM:GETTER_value

        return TYPE_SOA(Controller, value, ControllerValue);
      }
      void Controller::set_value(ControllerValue &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Controller> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_value
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_value

        // Set new value
        TYPE_SOA(Controller, value, ControllerValue) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_value
        // LOW_CODEGEN::END::CUSTOM:SETTER_value

        broadcast_observable(N(value));
      }

      ControllerType Controller::get_type() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Controller> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_type
        // LOW_CODEGEN::END::CUSTOM:GETTER_type

        return TYPE_SOA(Controller, type, ControllerType);
      }
      void Controller::set_type(ControllerType p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Controller> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_type
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_type

        // Set new value
        TYPE_SOA(Controller, type, ControllerType) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_type
        // LOW_CODEGEN::END::CUSTOM:SETTER_type

        broadcast_observable(N(type));
      }

      Low::Util::Name Controller::get_name() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Controller> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Controller, name, Low::Util::Name);
      }
      void Controller::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Controller> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Controller, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      Low::Core::UI::Controller
      Controller::make_code(Low::Util::Name p_Name)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_code
        Controller l_Controller = make(p_Name);
        l_Controller.set_type(ControllerType::Code);

        // TODO: Set value

        return l_Controller;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_code
      }

      Low::Core::UI::Controller
      Controller::make_script(Low::Util::Name p_Name,
                              Low::Core::Scripting::Class p_Class)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_script
        Controller l_Controller = make(p_Name);
        l_Controller.set_type(ControllerType::Script);

        l_Controller.get_value().script.sclass = p_Class;

        return l_Controller;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_script
      }

      Low::Core::UI::ControllerInstance Controller::spawn_instance()
      {
        Low::Util::HandleLock<Controller> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_spawn_instance
        ControllerInstance l_Instance =
            ControllerInstance::make(get_name());
        l_Instance.set_controller(get_id());

        if (is_script_controller()) {
          l_Instance.get_value().script.instance =
              get_value().script.sclass.spawn_instance(get_name());
        } else {
          LOW_ASSERT(false, "Unsupported controller type.");
        }

        return l_Instance;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_spawn_instance
      }

      Low::Core::UI::Controller Controller::find_by_scriptclass(
          Low::Core::Scripting::Class p_ScriptClass)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_find_by_scriptclass
        for (u32 i = 0; i < living_count(); ++i) {
          Controller i_Controller = living_instances()[i];

          if (i_Controller.is_script_controller() &&
              i_Controller.get_value().script.sclass ==
                  p_ScriptClass) {
            return i_Controller;
          }
        }
        return Util::Handle::DEAD;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_find_by_scriptclass
      }

      bool Controller::is_script_controller() const
      {
        Low::Util::HandleLock<Controller> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_script_controller
        return get_type() == ControllerType::Script;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_script_controller
      }

      void Controller::update_instances()
      {
        Low::Util::HandleLock<Controller> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update_instances
        LOW_NOT_IMPLEMENTED_WARN;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_update_instances
      }

      uint32_t Controller::create_instance(
          u32 &p_PageIndex, u32 &p_SlotIndex,
          Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock)
      {
        LOCK_PAGES_WRITE(l_PagesLock);
        u32 l_Index = 0;
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        bool l_FoundIndex = false;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;

        for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
             ++l_PageIndex) {
          Low::Util::UniqueLock<Low::Util::Mutex> i_PageLock(
              ms_Pages[l_PageIndex]->mutex);
          for (l_SlotIndex = 0;
               l_SlotIndex < ms_Pages[l_PageIndex]->size;
               ++l_SlotIndex) {
            if (!ms_Pages[l_PageIndex]
                     ->slots[l_SlotIndex]
                     .m_Occupied) {
              l_FoundIndex = true;
              l_PageLock = std::move(i_PageLock);
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
          Low::Util::UniqueLock<Low::Util::Mutex> l_NewLock(
              ms_Pages[l_PageIndex]->mutex);
          l_PageLock = std::move(l_NewLock);
        }
        ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
        p_PageIndex = l_PageIndex;
        p_SlotIndex = l_SlotIndex;
        p_PageLock = std::move(l_PageLock);
        LOCK_UNLOCK(l_PagesLock);
        return l_Index;
      }

      u32 Controller::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Controller.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Controller::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Controller::get_page_for_index(const u32 p_Index,
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

    } // namespace UI
  } // namespace Core
} // namespace Low
