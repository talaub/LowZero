#include "LowRendererShaderVariant.h"

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
#include "LowRendererShaderSource.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    u16 ShaderVariant::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        ShaderVariant::IDENTIFIER(LOW_NAME(509652687),
                                  LOW_NAME(2268062064));
    uint32_t ShaderVariant::ms_Capacity = 0u;
    uint32_t ShaderVariant::ms_PageSize = 0u;
    Low::Util::List<ShaderVariant> ShaderVariant::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        ShaderVariant::ms_Pages;

    Low::Util::Handle ShaderVariant::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    ShaderVariant ShaderVariant::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      ShaderVariant l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = ShaderVariant::ms_TypeId;

      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, ShaderVariant, dependent_pipelines,
          Low::Util::List<uint64_t>)) Low::Util::List<uint64_t>();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, ShaderVariant, defines,
          Low::Util::List<Low::Renderer::ShaderDefine>))
          Low::Util::List<Low::Renderer::ShaderDefine>();
      ACCESSOR_TYPE_SOA(l_Handle, ShaderVariant, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void ShaderVariant::destroy()
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

    void ShaderVariant::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowRenderer2), N(ShaderVariant));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(ShaderVariant));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, ShaderVariant::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(ShaderVariant);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &ShaderVariant::is_alive;
      l_TypeInfo.destroy = &ShaderVariant::destroy;
      l_TypeInfo.serialize = &ShaderVariant::serialize;
      l_TypeInfo.deserialize = &ShaderVariant::deserialize;
      l_TypeInfo.find_by_index = &ShaderVariant::_find_by_index;
      l_TypeInfo.notify = &ShaderVariant::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &ShaderVariant::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &ShaderVariant::_make;
      l_TypeInfo.duplicate_default = &ShaderVariant::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &ShaderVariant::living_instances);
      l_TypeInfo.get_living_count = &ShaderVariant::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: source_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(source_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ShaderVariant::Data, source_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ShaderVariant l_Handle = p_Handle.get_id();
          l_Handle.get_source_handle();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ShaderVariant,
                                            source_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ShaderVariant l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_source_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: source_handle
      }
      {
        // Property: entry_point
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(entry_point);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ShaderVariant::Data, entry_point);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ShaderVariant l_Handle = p_Handle.get_id();
          l_Handle.get_entry_point();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ShaderVariant,
                                            entry_point,
                                            Low::Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ShaderVariant l_Handle = p_Handle.get_id();
          l_Handle.set_entry_point(*(Low::Util::String *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ShaderVariant l_Handle = p_Handle.get_id();
          *((Low::Util::String *)p_Data) = l_Handle.get_entry_point();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: entry_point
      }
      {
        // Property: compiled_path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(compiled_path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ShaderVariant::Data, compiled_path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ShaderVariant l_Handle = p_Handle.get_id();
          l_Handle.get_compiled_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ShaderVariant,
                                            compiled_path,
                                            Low::Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ShaderVariant l_Handle = p_Handle.get_id();
          l_Handle.set_compiled_path(*(Low::Util::String *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ShaderVariant l_Handle = p_Handle.get_id();
          *((Low::Util::String *)p_Data) =
              l_Handle.get_compiled_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: compiled_path
      }
      {
        // Property: dependent_pipelines
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dependent_pipelines);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ShaderVariant::Data, dependent_pipelines);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ShaderVariant l_Handle = p_Handle.get_id();
          l_Handle.get_dependent_pipelines();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ShaderVariant, dependent_pipelines,
              Low::Util::List<uint64_t>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ShaderVariant l_Handle = p_Handle.get_id();
          l_Handle.set_dependent_pipelines(
              *(Low::Util::List<uint64_t> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ShaderVariant l_Handle = p_Handle.get_id();
          *((Low::Util::List<uint64_t> *)p_Data) =
              l_Handle.get_dependent_pipelines();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: dependent_pipelines
      }
      {
        // Property: defines
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(defines);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ShaderVariant::Data, defines);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ShaderVariant l_Handle = p_Handle.get_id();
          l_Handle.get_defines();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ShaderVariant, defines,
              Low::Util::List<Low::Renderer::ShaderDefine>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ShaderVariant l_Handle = p_Handle.get_id();
          l_Handle.set_defines(*(
              Low::Util::List<Low::Renderer::ShaderDefine> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ShaderVariant l_Handle = p_Handle.get_id();
          *((Low::Util::List<Low::Renderer::ShaderDefine> *)p_Data) =
              l_Handle.get_defines();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: defines
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ShaderVariant::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ShaderVariant l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ShaderVariant,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ShaderVariant l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ShaderVariant l_Handle = p_Handle.get_id();
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
        l_FunctionInfo.handleType = ShaderVariant::type_id();
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Source);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = ShaderSource::type_id();
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

    void ShaderVariant::cleanup()
    {
      Low::Util::List<ShaderVariant> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle ShaderVariant::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    ShaderVariant ShaderVariant::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      ShaderVariant l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = ShaderVariant::ms_TypeId;

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

    ShaderVariant ShaderVariant::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      ShaderVariant l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = ShaderVariant::ms_TypeId;

      return l_Handle;
    }

    bool ShaderVariant::is_alive() const
    {
      if (m_Data.m_Type != ShaderVariant::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == ShaderVariant::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t ShaderVariant::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    ShaderVariant::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    ShaderVariant ShaderVariant::find_by_name(Low::Util::Name p_Name)
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

    ShaderVariant
    ShaderVariant::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      ShaderVariant l_Handle = make(p_Name);
      l_Handle.set_source_handle(get_source_handle());
      l_Handle.set_entry_point(get_entry_point());
      l_Handle.set_compiled_path(get_compiled_path());
      l_Handle.set_dependent_pipelines(get_dependent_pipelines());
      l_Handle.set_defines(get_defines());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    ShaderVariant ShaderVariant::duplicate(ShaderVariant p_Handle,
                                           Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    ShaderVariant::_duplicate(Low::Util::Handle p_Handle,
                              Low::Util::Name p_Name)
    {
      ShaderVariant l_ShaderVariant = p_Handle.get_id();
      return l_ShaderVariant.duplicate(p_Name);
    }

    void
    ShaderVariant::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void ShaderVariant::serialize(Low::Util::Handle p_Handle,
                                  Low::Util::Serial::Node &p_Node)
    {
      ShaderVariant l_ShaderVariant = p_Handle.get_id();
      l_ShaderVariant.serialize(p_Node);
    }

    Low::Util::Handle
    ShaderVariant::deserialize(Low::Util::Serial::Node &p_Node,
                               Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void ShaderVariant::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 ShaderVariant::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 ShaderVariant::observe(Low::Util::Name p_Observable,
                               Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void ShaderVariant::notify(Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void ShaderVariant::_notify(Low::Util::Handle p_Observer,
                                Low::Util::Handle p_Observed,
                                Low::Util::Name p_Observable)
    {
      ShaderVariant l_ShaderVariant = p_Observer.get_id();
      l_ShaderVariant.notify(p_Observed, p_Observable);
    }

    uint64_t ShaderVariant::get_source_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_source_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_source_handle

      return TYPE_SOA(ShaderVariant, source_handle, uint64_t);
    }
    void ShaderVariant::set_source_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_source_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_source_handle

      // Set new value
      TYPE_SOA(ShaderVariant, source_handle, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_source_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_source_handle

      broadcast_observable(N(source_handle));
    }

    Low::Util::String ShaderVariant::get_entry_point() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entry_point
      // LOW_CODEGEN::END::CUSTOM:GETTER_entry_point

      return TYPE_SOA(ShaderVariant, entry_point, Low::Util::String);
    }
    void ShaderVariant::set_entry_point(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_entry_point(l_Val);
    }

    void ShaderVariant::set_entry_point(Low::Util::String p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entry_point
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_entry_point

      // Set new value
      TYPE_SOA(ShaderVariant, entry_point, Low::Util::String) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entry_point
      // LOW_CODEGEN::END::CUSTOM:SETTER_entry_point

      broadcast_observable(N(entry_point));
    }

    Low::Util::String ShaderVariant::get_compiled_path() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_compiled_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_compiled_path

      return TYPE_SOA(ShaderVariant, compiled_path,
                      Low::Util::String);
    }
    void ShaderVariant::set_compiled_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_compiled_path(l_Val);
    }

    void ShaderVariant::set_compiled_path(Low::Util::String p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_compiled_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_compiled_path

      // Set new value
      TYPE_SOA(ShaderVariant, compiled_path, Low::Util::String) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_compiled_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_compiled_path

      broadcast_observable(N(compiled_path));
    }

    Low::Util::List<uint64_t> &
    ShaderVariant::get_dependent_pipelines() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dependent_pipelines
      // LOW_CODEGEN::END::CUSTOM:GETTER_dependent_pipelines

      return TYPE_SOA(ShaderVariant, dependent_pipelines,
                      Low::Util::List<uint64_t>);
    }
    void ShaderVariant::set_dependent_pipelines(
        Low::Util::List<uint64_t> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dependent_pipelines
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dependent_pipelines

      // Set new value
      TYPE_SOA(ShaderVariant, dependent_pipelines,
               Low::Util::List<uint64_t>) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dependent_pipelines
      // LOW_CODEGEN::END::CUSTOM:SETTER_dependent_pipelines

      broadcast_observable(N(dependent_pipelines));
    }

    Low::Util::List<Low::Renderer::ShaderDefine> &
    ShaderVariant::get_defines() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_defines
      // LOW_CODEGEN::END::CUSTOM:GETTER_defines

      return TYPE_SOA(ShaderVariant, defines,
                      Low::Util::List<Low::Renderer::ShaderDefine>);
    }
    void ShaderVariant::set_defines(
        Low::Util::List<Low::Renderer::ShaderDefine> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_defines
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_defines

      // Set new value
      TYPE_SOA(ShaderVariant, defines,
               Low::Util::List<Low::Renderer::ShaderDefine>) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_defines
      // LOW_CODEGEN::END::CUSTOM:SETTER_defines

      broadcast_observable(N(defines));
    }

    Low::Util::Name ShaderVariant::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(ShaderVariant, name, Low::Util::Name);
    }
    void ShaderVariant::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(ShaderVariant, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    ShaderVariant ShaderVariant::make(Util::Name p_Name,
                                      ShaderSource p_Source)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      ShaderVariant l_ShaderVariant = make(p_Name);
      l_ShaderVariant.set_source_handle(p_Source.get_id());

      return l_ShaderVariant;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint32_t ShaderVariant::create_instance(u32 &p_PageIndex,
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

    u32 ShaderVariant::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for ShaderVariant.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, ShaderVariant::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool ShaderVariant::get_page_for_index(const u32 p_Index,
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

  } // namespace Renderer
} // namespace Low
