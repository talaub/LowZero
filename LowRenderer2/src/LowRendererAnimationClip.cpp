#include "LowRendererAnimationClip.h"

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
#include "LowUtilAssetManager.h"
#include "LowRendererResourceManager.h"
#include "LowRendererBase.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    u16 AnimationClip::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        AnimationClip::IDENTIFIER(LOW_NAME(509652687),
                                  LOW_NAME(794199450));
    uint32_t AnimationClip::ms_Capacity = 0u;
    uint32_t AnimationClip::ms_PageSize = 0u;
    Low::Util::List<AnimationClip> AnimationClip::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        AnimationClip::ms_Pages;

    Low::Util::Handle AnimationClip::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    AnimationClip AnimationClip::make(Low::Util::Name p_Name)
    {
      return make(p_Name, 0ull);
    }

    AnimationClip AnimationClip::make(Low::Util::Name p_Name,
                                      Low::Util::UniqueId p_UniqueId)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      AnimationClip l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = AnimationClip::ms_TypeId;

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, AnimationClip, state,
                                 Low::Renderer::AnimationClipState))
          Low::Renderer::AnimationClipState();
      new (
          ACCESSOR_TYPE_SOA_PTR(l_Handle, AnimationClip, resource,
                                Low::Renderer::AnimationClipResource))
          Low::Renderer::AnimationClipResource();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, AnimationClip, skeleton,
                                 Low::Renderer::Skeleton))
          Low::Renderer::Skeleton();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, AnimationClip, channels,
                                 Util::List<AnimationChannel>))
          Util::List<AnimationChannel>();
      ACCESSOR_TYPE_SOA(l_Handle, AnimationClip, duration, float) =
          0.0f;
      ACCESSOR_TYPE_SOA(l_Handle, AnimationClip, ticks_per_second,
                        float) = 0.0f;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, AnimationClip, references,
                                 Low::Util::Set<u64>))
          Low::Util::Set<u64>();
      ACCESSOR_TYPE_SOA(l_Handle, AnimationClip, name,
                        Low::Util::Name) = Low::Util::Name(0u);

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
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void AnimationClip::destroy()
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

    void AnimationClip::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowRenderer2), N(AnimationClip));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(AnimationClip));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, AnimationClip::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(AnimationClip);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &AnimationClip::is_alive;
      l_TypeInfo.destroy = &AnimationClip::destroy;
      l_TypeInfo.serialize = &AnimationClip::serialize;
      l_TypeInfo.deserialize = &AnimationClip::deserialize;
      l_TypeInfo.find_by_index = &AnimationClip::_find_by_index;
      l_TypeInfo.notify = &AnimationClip::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &AnimationClip::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &AnimationClip::_make;
      l_TypeInfo.duplicate_default = &AnimationClip::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &AnimationClip::living_instances);
      l_TypeInfo.get_living_count = &AnimationClip::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AnimationClip::Data, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType = Low::Renderer::
            AnimationClipStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AnimationClip l_Handle = p_Handle.get_id();
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, AnimationClip, state,
              Low::Renderer::AnimationClipState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          AnimationClip l_Handle = p_Handle.get_id();
          l_Handle.set_state(
              *(Low::Renderer::AnimationClipState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AnimationClip l_Handle = p_Handle.get_id();
          *((Low::Renderer::AnimationClipState *)p_Data) =
              l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
      {
        // Property: resource
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resource);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AnimationClip::Data, resource);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::AnimationClipResource::IDENTIFIER;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AnimationClip l_Handle = p_Handle.get_id();
          l_Handle.get_resource();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, AnimationClip, resource,
              Low::Renderer::AnimationClipResource);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          AnimationClip l_Handle = p_Handle.get_id();
          l_Handle.set_resource(
              *(Low::Renderer::AnimationClipResource *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AnimationClip l_Handle = p_Handle.get_id();
          *((Low::Renderer::AnimationClipResource *)p_Data) =
              l_Handle.get_resource();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: resource
      }
      {
        // Property: skeleton
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(skeleton);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AnimationClip::Data, skeleton);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::Skeleton::IDENTIFIER;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AnimationClip l_Handle = p_Handle.get_id();
          l_Handle.get_skeleton();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, AnimationClip,
                                            skeleton,
                                            Low::Renderer::Skeleton);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          AnimationClip l_Handle = p_Handle.get_id();
          l_Handle.set_skeleton(*(Low::Renderer::Skeleton *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AnimationClip l_Handle = p_Handle.get_id();
          *((Low::Renderer::Skeleton *)p_Data) =
              l_Handle.get_skeleton();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: skeleton
      }
      {
        // Property: channels
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(channels);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AnimationClip::Data, channels);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AnimationClip l_Handle = p_Handle.get_id();
          l_Handle.get_channels();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, AnimationClip, channels,
              Util::List<AnimationChannel>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AnimationClip l_Handle = p_Handle.get_id();
          *((Util::List<AnimationChannel> *)p_Data) =
              l_Handle.get_channels();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: channels
      }
      {
        // Property: duration
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(duration);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AnimationClip::Data, duration);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AnimationClip l_Handle = p_Handle.get_id();
          l_Handle.get_duration();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, AnimationClip,
                                            duration, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          AnimationClip l_Handle = p_Handle.get_id();
          l_Handle.set_duration(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AnimationClip l_Handle = p_Handle.get_id();
          *((float *)p_Data) = l_Handle.get_duration();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: duration
      }
      {
        // Property: ticks_per_second
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(ticks_per_second);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AnimationClip::Data, ticks_per_second);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AnimationClip l_Handle = p_Handle.get_id();
          l_Handle.get_ticks_per_second();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, AnimationClip,
                                            ticks_per_second, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          AnimationClip l_Handle = p_Handle.get_id();
          l_Handle.set_ticks_per_second(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AnimationClip l_Handle = p_Handle.get_id();
          *((float *)p_Data) = l_Handle.get_ticks_per_second();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: ticks_per_second
      }
      {
        // Property: references
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(references);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AnimationClip::Data, references);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: references
      }
      {
        // Property: unique_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(unique_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AnimationClip::Data, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AnimationClip l_Handle = p_Handle.get_id();
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, AnimationClip,
                                            unique_id,
                                            Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AnimationClip l_Handle = p_Handle.get_id();
          *((Low::Util::UniqueId *)p_Data) = l_Handle.get_unique_id();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: unique_id
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AnimationClip::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AnimationClip l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, AnimationClip,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          AnimationClip l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AnimationClip l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: make_from_resource_config
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make_from_resource_config);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = AnimationClip::type_id();
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Config);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make_from_resource_config
      }
      ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                        l_TypeInfo);
      // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
      {
        Util::AssetManager::TypeRegistratorBuilder l_Builder(
            N(AnimationClip), IDENTIFIER);
        l_Builder.auto_initialize(true)
            .initialize_on_startup(true)
            .add_asset_suffix(".animclipresource.yaml");

        l_Builder.add_initialize_directory(
            Util::get_project().assetCachePath, true);
        l_Builder.initializer([](const Util::String p_Path)
                                  -> Util::Handle {
          Util::Serial::Node l_ResourceNode =
              Util::Serial::load_yaml_file(p_Path.c_str());

          AnimationClipResourceConfig l_ResourceConfig;

          LOWR_ASSERT_RETURN(
              ResourceManager::parse_animation_clip_resource_config(
                  p_Path, l_ResourceNode, l_ResourceConfig),
              "Failed to parse animation clip resource config.");
          AnimationClip l_Existing =
              ResourceManager::find_asset<AnimationClip>(
                  l_ResourceConfig.animationclip_id);
          if (!l_Existing.is_alive()) {
            ResourceManager::register_asset(
                l_ResourceConfig.animationclip_id,
                AnimationClip::make_from_resource_config(
                    l_ResourceConfig));
          }
        });

        Util::AssetManager::register_asset_type(l_Builder.build());
      }
      // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
    }

    void AnimationClip::cleanup()
    {
      Low::Util::List<AnimationClip> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle AnimationClip::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    AnimationClip AnimationClip::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      AnimationClip l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = AnimationClip::ms_TypeId;

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

    AnimationClip AnimationClip::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      AnimationClip l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = AnimationClip::ms_TypeId;

      return l_Handle;
    }

    bool AnimationClip::is_alive() const
    {
      if (m_Data.m_Type != AnimationClip::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == AnimationClip::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t AnimationClip::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    AnimationClip::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    AnimationClip AnimationClip::find_by_name(Low::Util::Name p_Name)
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

    AnimationClip
    AnimationClip::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      AnimationClip l_Handle = make(p_Name);
      l_Handle.set_state(get_state());
      if (get_resource().is_alive()) {
        l_Handle.set_resource(get_resource());
      }
      if (get_skeleton().is_alive()) {
        l_Handle.set_skeleton(get_skeleton());
      }
      l_Handle.set_duration(get_duration());
      l_Handle.set_ticks_per_second(get_ticks_per_second());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    AnimationClip AnimationClip::duplicate(AnimationClip p_Handle,
                                           Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    AnimationClip::_duplicate(Low::Util::Handle p_Handle,
                              Low::Util::Name p_Name)
    {
      AnimationClip l_AnimationClip = p_Handle.get_id();
      return l_AnimationClip.duplicate(p_Name);
    }

    void
    AnimationClip::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      Low::Util::Serial::serialize_enum(
          p_Node["state"],
          Low::Renderer::AnimationClipStateEnumHelper::get_enum_id(),
          static_cast<uint8_t>(get_state()));
      if (get_resource().is_alive()) {
        get_resource().serialize(p_Node["resource"]);
      }
      if (get_skeleton().is_alive()) {
        get_skeleton().serialize(p_Node["skeleton"]);
      }
      p_Node["_unique_id"] = Low::Util::U64Id{get_unique_id()};
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void AnimationClip::serialize(Low::Util::Handle p_Handle,
                                  Low::Util::Serial::Node &p_Node)
    {
      AnimationClip l_AnimationClip = p_Handle.get_id();
      l_AnimationClip.serialize(p_Node);
    }

    Low::Util::Handle
    AnimationClip::deserialize(Low::Util::Serial::Node &p_Node,
                               Low::Util::Handle p_Creator)
    {
      Low::Util::UniqueId l_HandleUniqueId = 0ull;
      if (p_Node["unique_id"]) {
        l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
      } else if (p_Node["_unique_id"]) {
        l_HandleUniqueId = Low::Util::string_to_hash(
            p_Node["_unique_id"].as<Low::Util::String>());
      }

      AnimationClip l_Handle =
          AnimationClip::make(N(AnimationClip), l_HandleUniqueId);

      if (p_Node["state"]) {
        l_Handle.set_state(
            static_cast<Low::Renderer::AnimationClipState>(
                Low::Util::Serial::deserialize_enum(
                    p_Node["state"])));
      }
      if (p_Node["resource"]) {
        l_Handle.set_resource(
            Low::Renderer::AnimationClipResource::deserialize(
                p_Node["resource"], l_Handle.get_id())
                .get_id());
      }
      if (p_Node["skeleton"]) {
        l_Handle.set_skeleton(
            Low::Renderer::Skeleton::deserialize(p_Node["skeleton"],
                                                 l_Handle.get_id())
                .get_id());
      }
      if (p_Node["references"]) {
      }
      if (p_Node["unique_id"]) {
        l_Handle.set_unique_id(
            p_Node["unique_id"].as<Low::Util::UniqueId>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(p_Node["name"].as<Low::Util::Name>());
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void AnimationClip::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 AnimationClip::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 AnimationClip::observe(Low::Util::Name p_Observable,
                               Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void AnimationClip::notify(Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void AnimationClip::_notify(Low::Util::Handle p_Observer,
                                Low::Util::Handle p_Observed,
                                Low::Util::Name p_Observable)
    {
      AnimationClip l_AnimationClip = p_Observer.get_id();
      l_AnimationClip.notify(p_Observed, p_Observable);
    }

    void AnimationClip::reference(const u64 p_Id)
    {
      _LOW_ASSERT(is_alive());

      const u32 l_OldReferences =
          (TYPE_SOA(AnimationClip, references, Low::Util::Set<u64>))
              .size();

      (TYPE_SOA(AnimationClip, references, Low::Util::Set<u64>))
          .insert(p_Id);

      const u32 l_References =
          (TYPE_SOA(AnimationClip, references, Low::Util::Set<u64>))
              .size();

      if (l_OldReferences != l_References) {
        // LOW_CODEGEN:BEGIN:CUSTOM:NEW_REFERENCE
        // LOW_CODEGEN::END::CUSTOM:NEW_REFERENCE
      }
    }

    void AnimationClip::dereference(const u64 p_Id)
    {
      _LOW_ASSERT(is_alive());

      const u32 l_OldReferences =
          (TYPE_SOA(AnimationClip, references, Low::Util::Set<u64>))
              .size();

      (TYPE_SOA(AnimationClip, references, Low::Util::Set<u64>))
          .erase(p_Id);

      const u32 l_References =
          (TYPE_SOA(AnimationClip, references, Low::Util::Set<u64>))
              .size();

      if (l_OldReferences != l_References) {
        // LOW_CODEGEN:BEGIN:CUSTOM:REFERENCE_REMOVED
        // LOW_CODEGEN::END::CUSTOM:REFERENCE_REMOVED
      }
    }

    u32 AnimationClip::references() const
    {
      return get_references().size();
    }

    bool AnimationClip::is_referenced() const
    {
      return !get_references().empty();
    }

    Low::Renderer::AnimationClipState AnimationClip::get_state() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      return TYPE_SOA(AnimationClip, state,
                      Low::Renderer::AnimationClipState);
    }
    void AnimationClip::set_state(
        Low::Renderer::AnimationClipState p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      TYPE_SOA(AnimationClip, state,
               Low::Renderer::AnimationClipState) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    Low::Renderer::AnimationClipResource
    AnimationClip::get_resource() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resource
      // LOW_CODEGEN::END::CUSTOM:GETTER_resource

      return TYPE_SOA(AnimationClip, resource,
                      Low::Renderer::AnimationClipResource);
    }
    void AnimationClip::set_resource(
        Low::Renderer::AnimationClipResource p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_resource
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_resource

      // Set new value
      TYPE_SOA(AnimationClip, resource,
               Low::Renderer::AnimationClipResource) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_resource
      // LOW_CODEGEN::END::CUSTOM:SETTER_resource

      broadcast_observable(N(resource));
    }

    Low::Renderer::Skeleton AnimationClip::get_skeleton() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_skeleton
      // LOW_CODEGEN::END::CUSTOM:GETTER_skeleton

      return TYPE_SOA(AnimationClip, skeleton,
                      Low::Renderer::Skeleton);
    }
    void AnimationClip::set_skeleton(Low::Renderer::Skeleton p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_skeleton
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_skeleton

      // Set new value
      TYPE_SOA(AnimationClip, skeleton, Low::Renderer::Skeleton) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_skeleton
      // LOW_CODEGEN::END::CUSTOM:SETTER_skeleton

      broadcast_observable(N(skeleton));
    }

    Util::List<AnimationChannel> &AnimationClip::get_channels() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_channels
      // LOW_CODEGEN::END::CUSTOM:GETTER_channels

      return TYPE_SOA(AnimationClip, channels,
                      Util::List<AnimationChannel>);
    }

    float AnimationClip::get_duration() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_duration
      // LOW_CODEGEN::END::CUSTOM:GETTER_duration

      return TYPE_SOA(AnimationClip, duration, float);
    }
    void AnimationClip::set_duration(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_duration
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_duration

      // Set new value
      TYPE_SOA(AnimationClip, duration, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_duration
      // LOW_CODEGEN::END::CUSTOM:SETTER_duration

      broadcast_observable(N(duration));
    }

    float AnimationClip::get_ticks_per_second() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_ticks_per_second
      // LOW_CODEGEN::END::CUSTOM:GETTER_ticks_per_second

      return TYPE_SOA(AnimationClip, ticks_per_second, float);
    }
    void AnimationClip::set_ticks_per_second(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_ticks_per_second
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_ticks_per_second

      // Set new value
      TYPE_SOA(AnimationClip, ticks_per_second, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_ticks_per_second
      // LOW_CODEGEN::END::CUSTOM:SETTER_ticks_per_second

      broadcast_observable(N(ticks_per_second));
    }

    Low::Util::Set<u64> &AnimationClip::get_references() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_references
      // LOW_CODEGEN::END::CUSTOM:GETTER_references

      return TYPE_SOA(AnimationClip, references, Low::Util::Set<u64>);
    }

    Low::Util::UniqueId AnimationClip::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(AnimationClip, unique_id, Low::Util::UniqueId);
    }
    void AnimationClip::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(AnimationClip, unique_id, Low::Util::UniqueId) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

      broadcast_observable(N(unique_id));
    }

    Low::Util::Name AnimationClip::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(AnimationClip, name, Low::Util::Name);
    }
    void AnimationClip::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(AnimationClip, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    AnimationClip AnimationClip::make_from_resource_config(
        const AnimationClipResourceConfig &p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_from_resource_config
      AnimationClip l_Clip =
          make(p_Config.name, p_Config.animationclip_id);
      AnimationClipResource l_Resource =
          AnimationClipResource::make_from_config(p_Config);
      l_Clip.set_resource(l_Resource);
      l_Clip.set_skeleton(
          Renderer::ResourceManager::find_asset<Skeleton>(
              p_Config.skeleton_id));

      return l_Clip;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_from_resource_config
    }

    uint32_t AnimationClip::create_instance(u32 &p_PageIndex,
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

    u32 AnimationClip::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for AnimationClip.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, AnimationClip::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool AnimationClip::get_page_for_index(const u32 p_Index,
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
