#include "LowCoreScriptModule.h"

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
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Scripting {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 Module::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          Module::IDENTIFIER(LOW_NAME(1181529166),
                             LOW_NAME(193471262));
      uint32_t Module::ms_Capacity = 0u;
      uint32_t Module::ms_PageSize = 0u;
      Low::Util::List<Module> Module::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *> Module::ms_Pages;

      Low::Util::Handle Module::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Module Module::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        Module l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Module::ms_TypeId;

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Module, scripts,
                                   Low::Util::List<uint64_t>))
            Low::Util::List<uint64_t>();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Module, classes,
                                   Low::Util::List<uint64_t>))
            Low::Util::List<uint64_t>();
        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, Module, ticking_functions,
            Low::Util::List<char *>)) Low::Util::List<char *>();
        ACCESSOR_TYPE_SOA(l_Handle, Module, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        l_Handle.set_as_module(nullptr);
        l_Handle.set_reload_index(0);
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Module::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        _LOW_ASSERT(get_page_for_index(get_index(), l_PageIndex,
                                       l_SlotIndex));
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

      void Module::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(N(LowCore),
                                                          N(Module));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Module));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Module::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Module);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Module::is_alive;
        l_TypeInfo.destroy = &Module::destroy;
        l_TypeInfo.serialize = &Module::serialize;
        l_TypeInfo.deserialize = &Module::deserialize;
        l_TypeInfo.find_by_index = &Module::_find_by_index;
        l_TypeInfo.notify = &Module::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &Module::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Module::_make;
        l_TypeInfo.duplicate_default = &Module::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Module::living_instances);
        l_TypeInfo.get_living_count = &Module::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: scripts
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(scripts);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Module::Data, scripts);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Module l_Handle = p_Handle.get_id();
            l_Handle.get_scripts();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Module, scripts, Low::Util::List<uint64_t>);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Module l_Handle = p_Handle.get_id();
            *((Low::Util::List<uint64_t> *)p_Data) =
                l_Handle.get_scripts();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: scripts
        }
        {
          // Property: as_module
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(as_module);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Module::Data, as_module);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Module l_Handle = p_Handle.get_id();
            l_Handle.get_as_module();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Module,
                                              as_module, char *);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Module l_Handle = p_Handle.get_id();
            l_Handle.set_as_module(*(char **)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Module l_Handle = p_Handle.get_id();
            *((char **)p_Data) = l_Handle.get_as_module();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: as_module
        }
        {
          // Property: classes
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(classes);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Module::Data, classes);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Module l_Handle = p_Handle.get_id();
            l_Handle.get_classes();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Module, classes, Low::Util::List<uint64_t>);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Module l_Handle = p_Handle.get_id();
            *((Low::Util::List<uint64_t> *)p_Data) =
                l_Handle.get_classes();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: classes
        }
        {
          // Property: reload_index
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(reload_index);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Module::Data, reload_index);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Module l_Handle = p_Handle.get_id();
            l_Handle.get_reload_index();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Module,
                                              reload_index, uint32_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Module l_Handle = p_Handle.get_id();
            l_Handle.set_reload_index(*(uint32_t *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Module l_Handle = p_Handle.get_id();
            *((uint32_t *)p_Data) = l_Handle.get_reload_index();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: reload_index
        }
        {
          // Property: ticking_functions
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(ticking_functions);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Module::Data, ticking_functions);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Module l_Handle = p_Handle.get_id();
            l_Handle.get_ticking_functions();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Module, ticking_functions,
                Low::Util::List<char *>);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Module l_Handle = p_Handle.get_id();
            *((Low::Util::List<char *> *)p_Data) =
                l_Handle.get_ticking_functions();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: ticking_functions
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Module::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Module l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Module, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Module l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Module l_Handle = p_Handle.get_id();
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Function: find_class_by_name
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(find_class_by_name);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Name);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::NAME;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: find_class_by_name
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void Module::cleanup()
      {
        Low::Util::List<Module> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle Module::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Module Module::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Module l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Module::ms_TypeId;

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

      Module Module::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Module l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Module::ms_TypeId;

        return l_Handle;
      }

      bool Module::is_alive() const
      {
        if (m_Data.m_Type != Module::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == Module::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Module::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle Module::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      Module Module::find_by_name(Low::Util::Name p_Name)
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

      Module Module::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        LOW_ASSERT_WARN(false, "Not implemented");
        return 0;
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE
      }

      Module Module::duplicate(Module p_Handle,
                               Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle Module::_duplicate(Low::Util::Handle p_Handle,
                                           Low::Util::Name p_Name)
      {
        Module l_Module = p_Handle.get_id();
        return l_Module.duplicate(p_Name);
      }

      void Module::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Module::serialize(Low::Util::Handle p_Handle,
                             Low::Util::Serial::Node &p_Node)
      {
        Module l_Module = p_Handle.get_id();
        l_Module.serialize(p_Node);
      }

      Low::Util::Handle
      Module::deserialize(Low::Util::Serial::Node &p_Node,
                          Low::Util::Handle p_Creator)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        return Low::Util::Handle::DEAD;
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
      }

      void
      Module::broadcast_observable(Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 Module::observe(Low::Util::Name p_Observable,
                          Low::Util::Function<void(Low::Util::Handle,
                                                   Low::Util::Name)>
                              p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Module::observe(Low::Util::Name p_Observable,
                          Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Module::notify(Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Module::_notify(Low::Util::Handle p_Observer,
                           Low::Util::Handle p_Observed,
                           Low::Util::Name p_Observable)
      {
        Module l_Module = p_Observer.get_id();
        l_Module.notify(p_Observed, p_Observable);
      }

      Low::Util::List<uint64_t> &Module::get_scripts() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_scripts
        // LOW_CODEGEN::END::CUSTOM:GETTER_scripts

        return TYPE_SOA(Module, scripts, Low::Util::List<uint64_t>);
      }

      char *Module::get_as_module() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_as_module
        // LOW_CODEGEN::END::CUSTOM:GETTER_as_module

        return TYPE_SOA(Module, as_module, char *);
      }
      void Module::set_as_module(char *p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_as_module
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_as_module

        // Set new value
        TYPE_SOA(Module, as_module, char *) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_as_module
        // LOW_CODEGEN::END::CUSTOM:SETTER_as_module

        broadcast_observable(N(as_module));
      }

      Low::Util::List<uint64_t> &Module::get_classes() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_classes
        // LOW_CODEGEN::END::CUSTOM:GETTER_classes

        return TYPE_SOA(Module, classes, Low::Util::List<uint64_t>);
      }

      uint32_t Module::get_reload_index() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_reload_index
        // LOW_CODEGEN::END::CUSTOM:GETTER_reload_index

        return TYPE_SOA(Module, reload_index, uint32_t);
      }
      void Module::set_reload_index(uint32_t p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_reload_index
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_reload_index

        // Set new value
        TYPE_SOA(Module, reload_index, uint32_t) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_reload_index
        // LOW_CODEGEN::END::CUSTOM:SETTER_reload_index

        broadcast_observable(N(reload_index));
      }

      Low::Util::List<char *> &Module::get_ticking_functions() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_ticking_functions
        // LOW_CODEGEN::END::CUSTOM:GETTER_ticking_functions

        return TYPE_SOA(Module, ticking_functions,
                        Low::Util::List<char *>);
      }

      Low::Util::Name Module::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Module, name, Low::Util::Name);
      }
      void Module::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Module, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      uint64_t Module::find_class_by_name(Low::Util::Name p_Name)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_find_class_by_name
        for (u64 i_ClassId : get_classes()) {
          ScriptClass i_Class = i_ClassId;
          if (i_Class.get_name() == p_Name) {
            return i_ClassId;
          }
        }

        return Util::Handle::DEAD;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_find_class_by_name
      }

      uint32_t Module::create_instance(u32 &p_PageIndex,
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
            if (!ms_Pages[l_PageIndex]
                     ->slots[l_SlotIndex]
                     .m_Occupied) {
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

      u32 Module::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Module.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Module::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Module::get_page_for_index(const u32 p_Index,
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
