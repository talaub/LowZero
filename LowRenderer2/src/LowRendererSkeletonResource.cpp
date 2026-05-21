#include "LowRendererSkeletonResource.h"

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
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    u16 SkeletonResource::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        SkeletonResource::IDENTIFIER(LOW_NAME(509652687),
                                     LOW_NAME(1733632180));
    uint32_t SkeletonResource::ms_Capacity = 0u;
    uint32_t SkeletonResource::ms_PageSize = 0u;
    Low::Util::List<SkeletonResource>
        SkeletonResource::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        SkeletonResource::ms_Pages;

    Low::Util::Handle SkeletonResource::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    SkeletonResource SkeletonResource::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      SkeletonResource l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = SkeletonResource::ms_TypeId;

      ACCESSOR_TYPE_SOA(l_Handle, SkeletonResource, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void SkeletonResource::destroy()
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

    void SkeletonResource::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowRenderer2), N(SkeletonResource));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer2), N(SkeletonResource));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, SkeletonResource::Data::get_size(),
              ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(SkeletonResource);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &SkeletonResource::is_alive;
      l_TypeInfo.destroy = &SkeletonResource::destroy;
      l_TypeInfo.serialize = &SkeletonResource::serialize;
      l_TypeInfo.deserialize = &SkeletonResource::deserialize;
      l_TypeInfo.find_by_index = &SkeletonResource::_find_by_index;
      l_TypeInfo.notify = &SkeletonResource::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &SkeletonResource::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &SkeletonResource::_make;
      l_TypeInfo.duplicate_default = &SkeletonResource::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &SkeletonResource::living_instances);
      l_TypeInfo.get_living_count = &SkeletonResource::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletonResource::Data, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkeletonResource l_Handle = p_Handle.get_id();
          l_Handle.get_path();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SkeletonResource, path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkeletonResource l_Handle = p_Handle.get_id();
          *((Util::String *)p_Data) = l_Handle.get_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: path
      }
      {
        // Property: data_path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(data_path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletonResource::Data, data_path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkeletonResource l_Handle = p_Handle.get_id();
          l_Handle.get_data_path();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SkeletonResource, data_path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkeletonResource l_Handle = p_Handle.get_id();
          *((Util::String *)p_Data) = l_Handle.get_data_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: data_path
      }
      {
        // Property: skeleton_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(skeleton_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletonResource::Data, skeleton_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkeletonResource l_Handle = p_Handle.get_id();
          l_Handle.get_skeleton_id();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SkeletonResource, skeleton_id, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkeletonResource l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_skeleton_id();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: skeleton_id
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletonResource::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkeletonResource l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SkeletonResource, name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkeletonResource l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkeletonResource l_Handle = p_Handle.get_id();
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
        l_FunctionInfo.handleType = SkeletonResource::type_id();
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Path);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::STRING;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: find_by_path
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(find_by_path);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = SkeletonResource::type_id();
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Path);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::STRING;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: find_by_path
      }
      {
        // Function: make_from_config
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make_from_config);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = SkeletonResource::type_id();
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Config);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make_from_config
      }
      ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                        l_TypeInfo);
      // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
    }

    void SkeletonResource::cleanup()
    {
      Low::Util::List<SkeletonResource> l_Instances =
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
    SkeletonResource::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    SkeletonResource SkeletonResource::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      SkeletonResource l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = SkeletonResource::ms_TypeId;

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

    SkeletonResource
    SkeletonResource::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      SkeletonResource l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = SkeletonResource::ms_TypeId;

      return l_Handle;
    }

    bool SkeletonResource::is_alive() const
    {
      if (m_Data.m_Type != SkeletonResource::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == SkeletonResource::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t SkeletonResource::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    SkeletonResource::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    SkeletonResource
    SkeletonResource::find_by_name(Low::Util::Name p_Name)
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

    SkeletonResource
    SkeletonResource::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      SkeletonResource l_Handle = make(p_Name);
      l_Handle.set_path(get_path());
      l_Handle.set_data_path(get_data_path());
      l_Handle.set_skeleton_id(get_skeleton_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    SkeletonResource
    SkeletonResource::duplicate(SkeletonResource p_Handle,
                                Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    SkeletonResource::_duplicate(Low::Util::Handle p_Handle,
                                 Low::Util::Name p_Name)
    {
      SkeletonResource l_SkeletonResource = p_Handle.get_id();
      return l_SkeletonResource.duplicate(p_Name);
    }

    void
    SkeletonResource::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["path"] = get_path().c_str();
      p_Node["data_path"] = get_data_path().c_str();
      p_Node["skeleton_id"] = get_skeleton_id();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void SkeletonResource::serialize(Low::Util::Handle p_Handle,
                                     Low::Util::Serial::Node &p_Node)
    {
      SkeletonResource l_SkeletonResource = p_Handle.get_id();
      l_SkeletonResource.serialize(p_Node);
    }

    Low::Util::Handle
    SkeletonResource::deserialize(Low::Util::Serial::Node &p_Node,
                                  Low::Util::Handle p_Creator)
    {
      SkeletonResource l_Handle =
          SkeletonResource::make(N(SkeletonResource));

      if (p_Node["path"]) {
        l_Handle.set_path(p_Node["path"].as<Low::Util::String>());
      }
      if (p_Node["data_path"]) {
        l_Handle.set_data_path(
            p_Node["data_path"].as<Low::Util::String>());
      }
      if (p_Node["skeleton_id"]) {
        l_Handle.set_skeleton_id(
            p_Node["skeleton_id"].as<uint64_t>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(p_Node["name"].as<Low::Util::Name>());
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void SkeletonResource::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 SkeletonResource::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 SkeletonResource::observe(Low::Util::Name p_Observable,
                                  Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void SkeletonResource::notify(Low::Util::Handle p_Observed,
                                  Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void SkeletonResource::_notify(Low::Util::Handle p_Observer,
                                   Low::Util::Handle p_Observed,
                                   Low::Util::Name p_Observable)
    {
      SkeletonResource l_SkeletonResource = p_Observer.get_id();
      l_SkeletonResource.notify(p_Observed, p_Observable);
    }

    Util::String SkeletonResource::get_path() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      return TYPE_SOA(SkeletonResource, path, Util::String);
    }
    void SkeletonResource::set_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_path(l_Val);
    }

    void SkeletonResource::set_path(Util::String p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      TYPE_SOA(SkeletonResource, path, Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_path

      broadcast_observable(N(path));
    }

    Util::String SkeletonResource::get_data_path() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_data_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_data_path

      return TYPE_SOA(SkeletonResource, data_path, Util::String);
    }
    void SkeletonResource::set_data_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_data_path(l_Val);
    }

    void SkeletonResource::set_data_path(Util::String p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_data_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_data_path

      // Set new value
      TYPE_SOA(SkeletonResource, data_path, Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_data_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_data_path

      broadcast_observable(N(data_path));
    }

    uint64_t SkeletonResource::get_skeleton_id() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_skeleton_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_skeleton_id

      return TYPE_SOA(SkeletonResource, skeleton_id, uint64_t);
    }
    void SkeletonResource::set_skeleton_id(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_skeleton_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_skeleton_id

      // Set new value
      TYPE_SOA(SkeletonResource, skeleton_id, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_skeleton_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_skeleton_id

      broadcast_observable(N(skeleton_id));
    }

    Low::Util::Name SkeletonResource::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(SkeletonResource, name, Low::Util::Name);
    }
    void SkeletonResource::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(SkeletonResource, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    SkeletonResource SkeletonResource::make(Util::String p_Path)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_path() == p_Path) {
          return *it;
        }
      }

      Util::String l_FileName =
          p_Path.substr(p_Path.find_last_of("/\\") + 1);
      SkeletonResource l_Resource =
          SkeletonResource::make(LOW_NAME(l_FileName.c_str()));
      l_Resource.set_path(p_Path);

      return l_Resource;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    SkeletonResource
    SkeletonResource::find_by_path(Util::String p_Path)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_find_by_path
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_path() == p_Path) {
          return *it;
        }
      }

      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_find_by_path
    }

    SkeletonResource SkeletonResource::make_from_config(
        const SkeletonResourceConfig &p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_from_config
      SkeletonResource l_Resource = make(p_Config.name);
      l_Resource.set_path(p_Config.path);
      l_Resource.set_data_path(p_Config.data_path);
      l_Resource.set_skeleton_id(p_Config.skeleton_id);

      return l_Resource;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_from_config
    }

    uint32_t SkeletonResource::create_instance(u32 &p_PageIndex,
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

    u32 SkeletonResource::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for SkeletonResource.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, SkeletonResource::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool SkeletonResource::get_page_for_index(const u32 p_Index,
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
