#include "LowCoreScriptClassInstance.h"

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
#include "LowCoreScriptClass.h"
#include "LowCoreScripting.h"
#include <angelscript.h>
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Scripting {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 ClassInstance::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          ClassInstance::IDENTIFIER(LOW_NAME(1181529166),
                                    LOW_NAME(3104120572));
      uint32_t ClassInstance::ms_Capacity = 0u;
      uint32_t ClassInstance::ms_PageSize = 0u;
      Low::Util::SharedMutex ClassInstance::ms_LivingMutex;
      Low::Util::SharedMutex ClassInstance::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          ClassInstance::ms_PagesLock(ClassInstance::ms_PagesMutex,
                                      std::defer_lock);
      Low::Util::List<ClassInstance>
          ClassInstance::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          ClassInstance::ms_Pages;

      Low::Util::Handle ClassInstance::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      ClassInstance ClassInstance::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        ClassInstance l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = ClassInstance::ms_TypeId;

        l_PageLock.unlock();

        Low::Util::HandleLock<ClassInstance> l_HandleLock(l_Handle);

        ACCESSOR_TYPE_SOA(l_Handle, ClassInstance, name,
                          Low::Util::Name) = Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        {
          Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          ms_LivingInstances.push_back(l_Handle);
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void ClassInstance::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<ClassInstance> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          asIScriptObject *l_Object = (asIScriptObject *)get_ptr();
          l_Object->Release();
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

      void ClassInstance::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowCore), N(ClassInstance));

        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(
            N(LowCore), N(ClassInstance));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, ClassInstance::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(ClassInstance);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &ClassInstance::is_alive;
        l_TypeInfo.destroy = &ClassInstance::destroy;
        l_TypeInfo.serialize = &ClassInstance::serialize;
        l_TypeInfo.deserialize = &ClassInstance::deserialize;
        l_TypeInfo.find_by_index = &ClassInstance::_find_by_index;
        l_TypeInfo.notify = &ClassInstance::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &ClassInstance::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &ClassInstance::_make;
        l_TypeInfo.duplicate_default = &ClassInstance::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &ClassInstance::living_instances);
        l_TypeInfo.get_living_count = &ClassInstance::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: script_class
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(script_class);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ClassInstance::Data, script_class);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ClassInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ClassInstance> l_HandleLock(
                l_Handle);
            l_Handle.get_script_class();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ClassInstance,
                                              script_class, uint64_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ClassInstance l_Handle = p_Handle.get_id();
            l_Handle.set_script_class(*(uint64_t *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ClassInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ClassInstance> l_HandleLock(
                l_Handle);
            *((uint64_t *)p_Data) = l_Handle.get_script_class();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: script_class
        }
        {
          // Property: reload_index
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(reload_index);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ClassInstance::Data, reload_index);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ClassInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ClassInstance> l_HandleLock(
                l_Handle);
            l_Handle.get_reload_index();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ClassInstance,
                                              reload_index, uint32_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ClassInstance l_Handle = p_Handle.get_id();
            l_Handle.set_reload_index(*(uint32_t *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ClassInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ClassInstance> l_HandleLock(
                l_Handle);
            *((uint32_t *)p_Data) = l_Handle.get_reload_index();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: reload_index
        }
        {
          // Property: ptr
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(ptr);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ClassInstance::Data, ptr);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            return nullptr;
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ClassInstance l_Handle = p_Handle.get_id();
            l_Handle.set_ptr(*(char **)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: ptr
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ClassInstance::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ClassInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ClassInstance> l_HandleLock(
                l_Handle);
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ClassInstance,
                                              name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ClassInstance l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ClassInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<ClassInstance> l_HandleLock(
                l_Handle);
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Function: needs_refresh
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(needs_refresh);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: needs_refresh
        }
        {
          // Function: get_ptr
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_ptr);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_ptr
        }
        {
          // Function: make
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(make);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType =
              Low::Core::Scripting::ClassInstance::type_id();
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
            l_ParameterInfo.name = N(p_ScriptClass);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType =
                Low::Core::Scripting::Class::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: make
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void ClassInstance::cleanup()
      {
        Low::Util::List<ClassInstance> l_Instances =
            ms_LivingInstances;
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

      Low::Util::Handle
      ClassInstance::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      ClassInstance ClassInstance::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        ClassInstance l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = ClassInstance::ms_TypeId;

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

      ClassInstance ClassInstance::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        ClassInstance l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = ClassInstance::ms_TypeId;

        return l_Handle;
      }

      bool ClassInstance::is_alive() const
      {
        if (m_Data.m_Type != ClassInstance::ms_TypeId) {
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
        return m_Data.m_Type == ClassInstance::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t ClassInstance::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      ClassInstance::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      ClassInstance
      ClassInstance::find_by_name(Low::Util::Name p_Name)
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

      ClassInstance
      ClassInstance::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        LOW_ASSERT_WARN(false, "Not implemented");
        return 0;
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE
      }

      ClassInstance ClassInstance::duplicate(ClassInstance p_Handle,
                                             Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      ClassInstance::_duplicate(Low::Util::Handle p_Handle,
                                Low::Util::Name p_Name)
      {
        ClassInstance l_ClassInstance = p_Handle.get_id();
        return l_ClassInstance.duplicate(p_Name);
      }

      void
      ClassInstance::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void ClassInstance::serialize(Low::Util::Handle p_Handle,
                                    Low::Util::Serial::Node &p_Node)
      {
        ClassInstance l_ClassInstance = p_Handle.get_id();
        l_ClassInstance.serialize(p_Node);
      }

      Low::Util::Handle
      ClassInstance::deserialize(Low::Util::Serial::Node &p_Node,
                                 Low::Util::Handle p_Creator)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        return Util::Handle::DEAD;
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
      }

      void ClassInstance::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 ClassInstance::observe(
          Low::Util::Name p_Observable,
          Low::Util::Function<void(Low::Util::Handle,
                                   Low::Util::Name)>
              p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 ClassInstance::observe(Low::Util::Name p_Observable,
                                 Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void ClassInstance::notify(Low::Util::Handle p_Observed,
                                 Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void ClassInstance::_notify(Low::Util::Handle p_Observer,
                                  Low::Util::Handle p_Observed,
                                  Low::Util::Name p_Observable)
      {
        ClassInstance l_ClassInstance = p_Observer.get_id();
        l_ClassInstance.notify(p_Observed, p_Observable);
      }

      uint64_t ClassInstance::get_script_class() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ClassInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_script_class
        // LOW_CODEGEN::END::CUSTOM:GETTER_script_class

        return TYPE_SOA(ClassInstance, script_class, uint64_t);
      }
      void ClassInstance::set_script_class(uint64_t p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ClassInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_script_class
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_script_class

        // Set new value
        TYPE_SOA(ClassInstance, script_class, uint64_t) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_script_class
        // LOW_CODEGEN::END::CUSTOM:SETTER_script_class

        broadcast_observable(N(script_class));
      }

      uint32_t ClassInstance::get_reload_index() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ClassInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_reload_index
        // LOW_CODEGEN::END::CUSTOM:GETTER_reload_index

        return TYPE_SOA(ClassInstance, reload_index, uint32_t);
      }
      void ClassInstance::set_reload_index(uint32_t p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ClassInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_reload_index
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_reload_index

        // Set new value
        TYPE_SOA(ClassInstance, reload_index, uint32_t) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_reload_index
        // LOW_CODEGEN::END::CUSTOM:SETTER_reload_index

        broadcast_observable(N(reload_index));
      }

      char *ClassInstance::_ptr() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ClassInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_ptr
        // LOW_CODEGEN::END::CUSTOM:GETTER_ptr

        return TYPE_SOA(ClassInstance, ptr, char *);
      }
      void ClassInstance::set_ptr(char *p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ClassInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_ptr
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_ptr

        // Set new value
        TYPE_SOA(ClassInstance, ptr, char *) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_ptr
        // LOW_CODEGEN::END::CUSTOM:SETTER_ptr

        broadcast_observable(N(ptr));
      }

      Low::Util::Name ClassInstance::get_name() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ClassInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(ClassInstance, name, Low::Util::Name);
      }
      void ClassInstance::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<ClassInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(ClassInstance, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      bool ClassInstance::needs_refresh()
      {
        Low::Util::HandleLock<ClassInstance> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_needs_refresh
        ScriptClass l_Class = get_script_class();
        if (l_Class.needs_refresh()) {
          return true;
        }
        return l_Class.get_reload_index() != get_reload_index();
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_needs_refresh
      }

      char *ClassInstance::get_ptr()
      {
        Low::Util::HandleLock<ClassInstance> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_ptr
        LOW_ASSERT(!needs_refresh(),
                   "Class instance needs reload, implement me.");

        return _ptr();
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_ptr
      }

      Low::Core::Scripting::ClassInstance
      ClassInstance::make(Low::Util::Name p_Name,
                          Low::Core::Scripting::Class p_ScriptClass)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        LOW_ASSERT(p_ScriptClass.is_alive(),
                   "Cannot create instance of dead class.");
        LOW_ASSERT(p_ScriptClass.is_valid(),
                   "Cannot create instance of invalid script class.");

        ClassInstance l_Instance = make(p_Name);
        l_Instance.set_script_class(p_ScriptClass.get_id());
        l_Instance.set_reload_index(p_ScriptClass.get_reload_index());

        asITypeInfo *l_Type = (asITypeInfo *)p_ScriptClass.as_class();
        asIScriptEngine *l_Engine = l_Type->GetEngine();

        void *l_ObjRaw = l_Engine->CreateScriptObject(l_Type);
        LOW_ASSERT(l_ObjRaw,
                   "Failed to create script class instance.");

        asIScriptObject *l_Object =
            reinterpret_cast<asIScriptObject *>(l_ObjRaw);

        l_Instance.set_ptr((char *)l_Object);

        return l_Instance;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      uint32_t ClassInstance::create_instance(
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

      u32 ClassInstance::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for ClassInstance.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, ClassInstance::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool ClassInstance::get_page_for_index(const u32 p_Index,
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

    } // namespace Scripting
  } // namespace Core
} // namespace Low
