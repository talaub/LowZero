#include "LowRendererAnimationClipResource.h"

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

    u16 AnimationClipResource::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        AnimationClipResource::IDENTIFIER(LOW_NAME(509652687),
                                          LOW_NAME(3100616723));
    uint32_t AnimationClipResource::ms_Capacity = 0u;
    uint32_t AnimationClipResource::ms_PageSize = 0u;
    Low::Util::List<AnimationClipResource>
        AnimationClipResource::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        AnimationClipResource::ms_Pages;

    Low::Util::Handle
    AnimationClipResource::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    AnimationClipResource
    AnimationClipResource::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      AnimationClipResource l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = AnimationClipResource::ms_TypeId;

      ACCESSOR_TYPE_SOA(l_Handle, AnimationClipResource, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void AnimationClipResource::destroy()
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

    void AnimationClipResource::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowRenderer2), N(AnimationClipResource));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer2), N(AnimationClipResource));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, AnimationClipResource::Data::get_size(),
              ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(AnimationClipResource);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &AnimationClipResource::is_alive;
      l_TypeInfo.destroy = &AnimationClipResource::destroy;
      l_TypeInfo.serialize = &AnimationClipResource::serialize;
      l_TypeInfo.deserialize = &AnimationClipResource::deserialize;
      l_TypeInfo.find_by_index =
          &AnimationClipResource::_find_by_index;
      l_TypeInfo.notify = &AnimationClipResource::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &AnimationClipResource::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &AnimationClipResource::_make;
      l_TypeInfo.duplicate_default =
          &AnimationClipResource::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &AnimationClipResource::living_instances);
      l_TypeInfo.get_living_count =
          &AnimationClipResource::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AnimationClipResource::Data, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AnimationClipResource l_Handle = p_Handle.get_id();
          l_Handle.get_path();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, AnimationClipResource, path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AnimationClipResource l_Handle = p_Handle.get_id();
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
            offsetof(AnimationClipResource::Data, data_path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AnimationClipResource l_Handle = p_Handle.get_id();
          l_Handle.get_data_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle,
                                            AnimationClipResource,
                                            data_path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AnimationClipResource l_Handle = p_Handle.get_id();
          *((Util::String *)p_Data) = l_Handle.get_data_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: data_path
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AnimationClipResource::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AnimationClipResource l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, AnimationClipResource, name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          AnimationClipResource l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AnimationClipResource l_Handle = p_Handle.get_id();
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
        l_FunctionInfo.handleType = AnimationClipResource::type_id();
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
        l_FunctionInfo.handleType = AnimationClipResource::type_id();
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
        l_FunctionInfo.handleType = AnimationClipResource::type_id();
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

    void AnimationClipResource::cleanup()
    {
      Low::Util::List<AnimationClipResource> l_Instances =
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
    AnimationClipResource::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    AnimationClipResource
    AnimationClipResource::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      AnimationClipResource l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = AnimationClipResource::ms_TypeId;

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

    AnimationClipResource
    AnimationClipResource::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      AnimationClipResource l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = AnimationClipResource::ms_TypeId;

      return l_Handle;
    }

    bool AnimationClipResource::is_alive() const
    {
      if (m_Data.m_Type != AnimationClipResource::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == AnimationClipResource::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t AnimationClipResource::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    AnimationClipResource::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    AnimationClipResource
    AnimationClipResource::find_by_name(Low::Util::Name p_Name)
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

    AnimationClipResource
    AnimationClipResource::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      AnimationClipResource l_Handle = make(p_Name);
      l_Handle.set_path(get_path());
      l_Handle.set_data_path(get_data_path());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    AnimationClipResource
    AnimationClipResource::duplicate(AnimationClipResource p_Handle,
                                     Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    AnimationClipResource::_duplicate(Low::Util::Handle p_Handle,
                                      Low::Util::Name p_Name)
    {
      AnimationClipResource l_AnimationClipResource =
          p_Handle.get_id();
      return l_AnimationClipResource.duplicate(p_Name);
    }

    void AnimationClipResource::serialize(
        Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["path"] = get_path().c_str();
      p_Node["data_path"] = get_data_path().c_str();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void
    AnimationClipResource::serialize(Low::Util::Handle p_Handle,
                                     Low::Util::Serial::Node &p_Node)
    {
      AnimationClipResource l_AnimationClipResource =
          p_Handle.get_id();
      l_AnimationClipResource.serialize(p_Node);
    }

    Low::Util::Handle AnimationClipResource::deserialize(
        Low::Util::Serial::Node &p_Node, Low::Util::Handle p_Creator)
    {
      AnimationClipResource l_Handle =
          AnimationClipResource::make(N(AnimationClipResource));

      if (p_Node["path"]) {
        l_Handle.set_path(p_Node["path"].as<Low::Util::String>());
      }
      if (p_Node["data_path"]) {
        l_Handle.set_data_path(
            p_Node["data_path"].as<Low::Util::String>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(p_Node["name"].as<Low::Util::Name>());
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void AnimationClipResource::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 AnimationClipResource::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64
    AnimationClipResource::observe(Low::Util::Name p_Observable,
                                   Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void AnimationClipResource::notify(Low::Util::Handle p_Observed,
                                       Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void AnimationClipResource::_notify(Low::Util::Handle p_Observer,
                                        Low::Util::Handle p_Observed,
                                        Low::Util::Name p_Observable)
    {
      AnimationClipResource l_AnimationClipResource =
          p_Observer.get_id();
      l_AnimationClipResource.notify(p_Observed, p_Observable);
    }

    Util::String AnimationClipResource::get_path() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      return TYPE_SOA(AnimationClipResource, path, Util::String);
    }
    void AnimationClipResource::set_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_path(l_Val);
    }

    void AnimationClipResource::set_path(Util::String p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      TYPE_SOA(AnimationClipResource, path, Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_path

      broadcast_observable(N(path));
    }

    Util::String AnimationClipResource::get_data_path() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_data_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_data_path

      return TYPE_SOA(AnimationClipResource, data_path, Util::String);
    }
    void AnimationClipResource::set_data_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_data_path(l_Val);
    }

    void AnimationClipResource::set_data_path(Util::String p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_data_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_data_path

      // Set new value
      TYPE_SOA(AnimationClipResource, data_path, Util::String) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_data_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_data_path

      broadcast_observable(N(data_path));
    }

    Low::Util::Name AnimationClipResource::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(AnimationClipResource, name, Low::Util::Name);
    }
    void AnimationClipResource::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(AnimationClipResource, name, Low::Util::Name) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    AnimationClipResource
    AnimationClipResource::make(Util::String p_Path)
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
      AnimationClipResource l_Resource =
          AnimationClipResource::make(LOW_NAME(l_FileName.c_str()));
      l_Resource.set_path(p_Path);

      return l_Resource;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    AnimationClipResource
    AnimationClipResource::find_by_path(Util::String p_Path)
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

    AnimationClipResource AnimationClipResource::make_from_config(
        const AnimationClipResourceConfig &p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_from_config
      AnimationClipResource l_Resource = make(p_Config.name);
      l_Resource.set_path(p_Config.path);
      l_Resource.set_data_path(p_Config.data_path);

      return l_Resource;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_from_config
    }

    uint32_t AnimationClipResource::create_instance(u32 &p_PageIndex,
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

    u32 AnimationClipResource::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT(
          (l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
          "Could not increase capacity for AnimationClipResource.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, AnimationClipResource::Data::get_size(),
          ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool AnimationClipResource::get_page_for_index(const u32 p_Index,
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
