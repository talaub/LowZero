#include "LowRendererSkeleton.h"

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
#include "LowRendererBase.h"
#include "LowRendererResourceManager.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    u16 Skeleton::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        Skeleton::IDENTIFIER(LOW_NAME(509652687),
                             LOW_NAME(615435132));
    uint32_t Skeleton::ms_Capacity = 0u;
    uint32_t Skeleton::ms_PageSize = 0u;
    Low::Util::List<Skeleton> Skeleton::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *> Skeleton::ms_Pages;

    Low::Util::Handle Skeleton::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Skeleton Skeleton::make(Low::Util::Name p_Name)
    {
      return make(p_Name, 0ull);
    }

    Skeleton Skeleton::make(Low::Util::Name p_Name,
                            Low::Util::UniqueId p_UniqueId)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      Skeleton l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = Skeleton::ms_TypeId;

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Skeleton, state,
                                 Low::Renderer::SkeletonState))
          Low::Renderer::SkeletonState();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Skeleton, resource,
                                 Low::Renderer::SkeletonResource))
          Low::Renderer::SkeletonResource();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Skeleton, bones,
                                 Util::List<SkeletonBone>))
          Util::List<SkeletonBone>();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, Skeleton, bone_map,
          SINGLE_ARG(Util::Map<Util::Name, uint32_t>)))
          Util::Map<Util::Name, uint32_t>();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Skeleton, references,
                                 Low::Util::Set<u64>))
          Low::Util::Set<u64>();
      ACCESSOR_TYPE_SOA(l_Handle, Skeleton, name, Low::Util::Name) =
          Low::Util::Name(0u);

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

    void Skeleton::destroy()
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

    void Skeleton::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowRenderer2), N(Skeleton));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(Skeleton));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, Skeleton::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Skeleton);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Skeleton::is_alive;
      l_TypeInfo.destroy = &Skeleton::destroy;
      l_TypeInfo.serialize = &Skeleton::serialize;
      l_TypeInfo.deserialize = &Skeleton::deserialize;
      l_TypeInfo.find_by_index = &Skeleton::_find_by_index;
      l_TypeInfo.notify = &Skeleton::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &Skeleton::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Skeleton::_make;
      l_TypeInfo.duplicate_default = &Skeleton::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Skeleton::living_instances);
      l_TypeInfo.get_living_count = &Skeleton::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Skeleton::Data, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            Low::Renderer::SkeletonStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Skeleton, state,
              Low::Renderer::SkeletonState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(Low::Renderer::SkeletonState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Skeleton l_Handle = p_Handle.get_id();
          *((Low::Renderer::SkeletonState *)p_Data) =
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
            offsetof(Skeleton::Data, resource);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::SkeletonResource::IDENTIFIER;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.get_resource();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Skeleton, resource,
              Low::Renderer::SkeletonResource);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.set_resource(
              *(Low::Renderer::SkeletonResource *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Skeleton l_Handle = p_Handle.get_id();
          *((Low::Renderer::SkeletonResource *)p_Data) =
              l_Handle.get_resource();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: resource
      }
      {
        // Property: bones
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(bones);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Skeleton::Data, bones);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.get_bones();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Skeleton, bones,
                                            Util::List<SkeletonBone>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Skeleton l_Handle = p_Handle.get_id();
          *((Util::List<SkeletonBone> *)p_Data) =
              l_Handle.get_bones();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: bones
      }
      {
        // Property: bone_map
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(bone_map);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Skeleton::Data, bone_map);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.get_bone_map();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Skeleton, bone_map,
              SINGLE_ARG(Util::Map<Util::Name, uint32_t>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Skeleton l_Handle = p_Handle.get_id();
          *((Util::Map<Util::Name, uint32_t> *)p_Data) =
              l_Handle.get_bone_map();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: bone_map
      }
      {
        // Property: bone_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(bone_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Skeleton::Data, bone_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.get_bone_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Skeleton,
                                            bone_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Skeleton l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_bone_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: bone_count
      }
      {
        // Property: references
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(references);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Skeleton::Data, references);
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
            offsetof(Skeleton::Data, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Skeleton, unique_id, Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Skeleton l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(Skeleton::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Skeleton, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Skeleton l_Handle = p_Handle.get_id();
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
        l_FunctionInfo.handleType = Skeleton::type_id();
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
      {
        // Function: find_bone_by_name
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(find_bone_by_name);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: find_bone_by_name
      }
      ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                        l_TypeInfo);
      // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
      {
        Util::AssetManager::TypeRegistratorBuilder l_Builder(
            N(Skeleton), IDENTIFIER);
        l_Builder.auto_initialize(true)
            .initialize_on_startup(true)
            .add_asset_suffix(".skeletonresource.yaml");

        l_Builder.add_initialize_directory(
            Util::get_project().dataPath, true);
        l_Builder.initializer(
            [](const Util::String p_Path) -> Util::Handle {
              Util::Serial::Node l_ResourceNode =
                  Util::Serial::load_yaml_file(p_Path.c_str());

              SkeletonResourceConfig l_ResourceConfig;

              LOWR_ASSERT_RETURN(
                  ResourceManager::parse_skeleton_resource_config(
                      p_Path, l_ResourceNode, l_ResourceConfig),
                  "Failed to parse skeleton resource config.");
              Skeleton l_Existing =
                  ResourceManager::find_asset<Skeleton>(
                      l_ResourceConfig.skeleton_id);
              if (!l_Existing.is_alive()) {
                ResourceManager::register_asset(
                    l_ResourceConfig.skeleton_id,
                    Skeleton::make_from_resource_config(
                        l_ResourceConfig));
              }
            });

        Util::AssetManager::register_asset_type(l_Builder.build());
      }
      // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
    }

    void Skeleton::cleanup()
    {
      Low::Util::List<Skeleton> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle Skeleton::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    Skeleton Skeleton::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Skeleton l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = Skeleton::ms_TypeId;

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

    Skeleton Skeleton::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      Skeleton l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = Skeleton::ms_TypeId;

      return l_Handle;
    }

    bool Skeleton::is_alive() const
    {
      if (m_Data.m_Type != Skeleton::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == Skeleton::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t Skeleton::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle Skeleton::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    Skeleton Skeleton::find_by_name(Low::Util::Name p_Name)
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

    Skeleton Skeleton::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      Skeleton l_Handle = make(p_Name);
      l_Handle.set_state(get_state());
      if (get_resource().is_alive()) {
        l_Handle.set_resource(get_resource());
      }
      l_Handle.set_bone_count(get_bone_count());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    Skeleton Skeleton::duplicate(Skeleton p_Handle,
                                 Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle Skeleton::_duplicate(Low::Util::Handle p_Handle,
                                           Low::Util::Name p_Name)
    {
      Skeleton l_Skeleton = p_Handle.get_id();
      return l_Skeleton.duplicate(p_Name);
    }

    void Skeleton::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      Low::Util::Serial::serialize_enum(
          p_Node["state"],
          Low::Renderer::SkeletonStateEnumHelper::get_enum_id(),
          static_cast<uint8_t>(get_state()));
      if (get_resource().is_alive()) {
        get_resource().serialize(p_Node["resource"]);
      }
      p_Node["bone_count"] = get_bone_count();
      p_Node["_unique_id"] = Low::Util::U64Id{get_unique_id()};
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Skeleton::serialize(Low::Util::Handle p_Handle,
                             Low::Util::Serial::Node &p_Node)
    {
      Skeleton l_Skeleton = p_Handle.get_id();
      l_Skeleton.serialize(p_Node);
    }

    Low::Util::Handle
    Skeleton::deserialize(Low::Util::Serial::Node &p_Node,
                          Low::Util::Handle p_Creator)
    {
      Low::Util::UniqueId l_HandleUniqueId = 0ull;
      if (p_Node["unique_id"]) {
        l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
      } else if (p_Node["_unique_id"]) {
        l_HandleUniqueId = Low::Util::string_to_hash(
            p_Node["_unique_id"].as<Low::Util::String>());
      }

      Skeleton l_Handle =
          Skeleton::make(N(Skeleton), l_HandleUniqueId);

      if (p_Node["state"]) {
        l_Handle.set_state(static_cast<Low::Renderer::SkeletonState>(
            Low::Util::Serial::deserialize_enum(p_Node["state"])));
      }
      if (p_Node["resource"]) {
        l_Handle.set_resource(
            Low::Renderer::SkeletonResource::deserialize(
                p_Node["resource"], l_Handle.get_id())
                .get_id());
      }
      if (p_Node["bones"]) {
      }
      if (p_Node["bone_map"]) {
      }
      if (p_Node["bone_count"]) {
        l_Handle.set_bone_count(p_Node["bone_count"].as<uint32_t>());
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

    void
    Skeleton::broadcast_observable(Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 Skeleton::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 Skeleton::observe(Low::Util::Name p_Observable,
                          Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void Skeleton::notify(Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void Skeleton::_notify(Low::Util::Handle p_Observer,
                           Low::Util::Handle p_Observed,
                           Low::Util::Name p_Observable)
    {
      Skeleton l_Skeleton = p_Observer.get_id();
      l_Skeleton.notify(p_Observed, p_Observable);
    }

    void Skeleton::reference(const u64 p_Id)
    {
      _LOW_ASSERT(is_alive());

      const u32 l_OldReferences =
          (TYPE_SOA(Skeleton, references, Low::Util::Set<u64>))
              .size();

      (TYPE_SOA(Skeleton, references, Low::Util::Set<u64>))
          .insert(p_Id);

      const u32 l_References =
          (TYPE_SOA(Skeleton, references, Low::Util::Set<u64>))
              .size();

      if (l_OldReferences != l_References) {
        // LOW_CODEGEN:BEGIN:CUSTOM:NEW_REFERENCE
        // LOW_CODEGEN::END::CUSTOM:NEW_REFERENCE
      }
    }

    void Skeleton::dereference(const u64 p_Id)
    {
      _LOW_ASSERT(is_alive());

      const u32 l_OldReferences =
          (TYPE_SOA(Skeleton, references, Low::Util::Set<u64>))
              .size();

      (TYPE_SOA(Skeleton, references, Low::Util::Set<u64>))
          .erase(p_Id);

      const u32 l_References =
          (TYPE_SOA(Skeleton, references, Low::Util::Set<u64>))
              .size();

      if (l_OldReferences != l_References) {
        // LOW_CODEGEN:BEGIN:CUSTOM:REFERENCE_REMOVED
        // LOW_CODEGEN::END::CUSTOM:REFERENCE_REMOVED
      }
    }

    u32 Skeleton::references() const
    {
      return get_references().size();
    }

    bool Skeleton::is_referenced() const
    {
      return !get_references().empty();
    }

    Low::Renderer::SkeletonState Skeleton::get_state() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      return TYPE_SOA(Skeleton, state, Low::Renderer::SkeletonState);
    }
    void Skeleton::set_state(Low::Renderer::SkeletonState p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      TYPE_SOA(Skeleton, state, Low::Renderer::SkeletonState) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    Low::Renderer::SkeletonResource Skeleton::get_resource() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resource
      // LOW_CODEGEN::END::CUSTOM:GETTER_resource

      return TYPE_SOA(Skeleton, resource,
                      Low::Renderer::SkeletonResource);
    }
    void
    Skeleton::set_resource(Low::Renderer::SkeletonResource p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_resource
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_resource

      // Set new value
      TYPE_SOA(Skeleton, resource, Low::Renderer::SkeletonResource) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_resource
      // LOW_CODEGEN::END::CUSTOM:SETTER_resource

      broadcast_observable(N(resource));
    }

    Util::List<SkeletonBone> &Skeleton::get_bones() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_bones
      // LOW_CODEGEN::END::CUSTOM:GETTER_bones

      return TYPE_SOA(Skeleton, bones, Util::List<SkeletonBone>);
    }

    Util::Map<Util::Name, uint32_t> &Skeleton::get_bone_map() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_bone_map
      // LOW_CODEGEN::END::CUSTOM:GETTER_bone_map

      return TYPE_SOA(Skeleton, bone_map,
                      SINGLE_ARG(Util::Map<Util::Name, uint32_t>));
    }

    uint32_t Skeleton::get_bone_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_bone_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_bone_count

      return TYPE_SOA(Skeleton, bone_count, uint32_t);
    }
    void Skeleton::set_bone_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_bone_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_bone_count

      // Set new value
      TYPE_SOA(Skeleton, bone_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_bone_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_bone_count

      broadcast_observable(N(bone_count));
    }

    Low::Util::Set<u64> &Skeleton::get_references() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_references
      // LOW_CODEGEN::END::CUSTOM:GETTER_references

      return TYPE_SOA(Skeleton, references, Low::Util::Set<u64>);
    }

    Low::Util::UniqueId Skeleton::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(Skeleton, unique_id, Low::Util::UniqueId);
    }
    void Skeleton::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(Skeleton, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

      broadcast_observable(N(unique_id));
    }

    Low::Util::Name Skeleton::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Skeleton, name, Low::Util::Name);
    }
    void Skeleton::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Skeleton, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    Skeleton Skeleton::make_from_resource_config(
        const SkeletonResourceConfig &p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_from_resource_config
      Skeleton l_Skeleton = make(p_Config.name, p_Config.skeleton_id);
      SkeletonResource l_Resource =
          SkeletonResource::make_from_config(p_Config);
      l_Skeleton.set_resource(l_Resource);
      l_Skeleton.set_bone_count(p_Config.bone_count);

      return l_Skeleton;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_from_resource_config
    }

    const SkeletonBone &Skeleton::find_bone_by_name(Util::Name p_Name)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_find_bone_by_name
      auto l_Pos = get_bone_map().find(p_Name);
      LOW_ASSERT(l_Pos != get_bone_map().end(),
                 "Could not find bone in skeleton.");

      return get_bones()[l_Pos->second];
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_find_bone_by_name
    }

    uint32_t Skeleton::create_instance(u32 &p_PageIndex,
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

    u32 Skeleton::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for Skeleton.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, Skeleton::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool Skeleton::get_page_for_index(const u32 p_Index,
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
