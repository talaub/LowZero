#include "LowCoreNavigationSource.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowCorePrefabInstance.h"
// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowCoreNavigation.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Navigation {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      static void observe_component(Source p_Source,
                                    Low::Util::Handle p_Component,
                                    Low::Util::Name p_Observable)
      {
        if (!p_Source.is_alive()) {
          return;
        }

        Low::Util::RTTI::TypeInfo &l_TypeInfo =
            Low::Util::Handle::get_type_info(p_Component.get_type());
        if (!l_TypeInfo.is_alive(p_Component)) {
          return;
        }

        Low::Util::ObserverKey l_Key;
        l_Key.handleId = p_Component.get_id();
        l_Key.observableName = p_Observable.m_Index;
        Low::Util::observe(l_Key, p_Source);
      }

      static void register_source_dirty_observers(Source p_Source)
      {
        if (!p_Source.is_alive()) {
          return;
        }

        Low::Core::Entity l_Entity = p_Source.get_entity();
        if (!l_Entity.is_alive()) {
          return;
        }

        for (auto &i_Component : l_Entity.get_components()) {
          Low::Util::Handle i_Handle = i_Component.second;
          if (i_Handle.get_id() == p_Source.get_id()) {
            continue;
          }
          observe_component(p_Source, i_Handle, N(dirty));
          observe_component(p_Source, i_Handle, N(world_dirty));
        }
      }
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 Source::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          Source::IDENTIFIER(LOW_NAME(1181529166),
                             LOW_NAME(2065530209));
      uint32_t Source::ms_Capacity = 0u;
      uint32_t Source::ms_PageSize = 0u;
      Low::Util::List<Source> Source::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *> Source::ms_Pages;

      Low::Util::Handle Source::_make(Low::Util::Handle p_Entity)
      {
        Low::Core::Entity l_Entity = p_Entity.get_id();
        LOW_ASSERT(l_Entity.is_alive(),
                   "Cannot create component for dead entity");
        return make(l_Entity).get_id();
      }

      Source Source::make(Low::Core::Entity p_Entity)
      {
        return make(p_Entity, 0ull);
      }

      Source Source::make(Low::Core::Entity p_Entity,
                          Low::Util::UniqueId p_UniqueId)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        Source l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Source::ms_TypeId;

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Source, mode,
                                   SourceMode)) SourceMode();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Source, geometry_type,
                                   SourceGeometryType))
            SourceGeometryType();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Source, area_type,
                                   AreaType)) AreaType();
        ACCESSOR_TYPE_SOA(l_Handle, Source, include_children, bool) =
            false;
        ACCESSOR_TYPE_SOA(l_Handle, Source, tile_dirty, bool) = false;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Source, bounds,
                                   Low::Math::Bounds))
            Low::Math::Bounds();
        ACCESSOR_TYPE_SOA(l_Handle, Source, bounds_valid, bool) =
            false;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Source, entity,
                                   Low::Core::Entity))
            Low::Core::Entity();

        l_Handle.set_entity(p_Entity);
        p_Entity.add_component(l_Handle);

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
        l_Handle.set_mode(SourceMode::SURFACE);
        l_Handle.set_geometry_type(SourceGeometryType::COLLIDER);
        l_Handle.set_area_type(AreaType::NORMAL);
        l_Handle.set_agent_mask(1u);
        register_source_dirty_observers(l_Handle);
        l_Handle.mark_dirty();
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Source::destroy()
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

      void Source::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowCore), N(NavigationSource));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Source));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Source::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Source);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Source::is_alive;
        l_TypeInfo.destroy = &Source::destroy;
        l_TypeInfo.serialize = &Source::serialize;
        l_TypeInfo.deserialize = &Source::deserialize;
        l_TypeInfo.find_by_index = &Source::_find_by_index;
        l_TypeInfo.notify = &Source::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.make_default = nullptr;
        l_TypeInfo.make_component = &Source::_make;
        l_TypeInfo.duplicate_default = nullptr;
        l_TypeInfo.duplicate_component = &Source::_duplicate;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Source::living_instances);
        l_TypeInfo.get_living_count = &Source::living_count;
        l_TypeInfo.component = true;
        l_TypeInfo.uiComponent = false;
        {
          // Property: mode
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(mode);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(Source::Data, mode);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
          l_PropertyInfo.handleType =
              SourceModeEnumHelper::get_enum_id();
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Source l_Handle = p_Handle.get_id();
            l_Handle.get_mode();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Source, mode,
                                              SourceMode);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Source l_Handle = p_Handle.get_id();
            l_Handle.set_mode(*(SourceMode *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Source l_Handle = p_Handle.get_id();
            *((SourceMode *)p_Data) = l_Handle.get_mode();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: mode
        }
        {
          // Property: geometry_type
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(geometry_type);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(Source::Data, geometry_type);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
          l_PropertyInfo.handleType =
              SourceGeometryTypeEnumHelper::get_enum_id();
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Source l_Handle = p_Handle.get_id();
            l_Handle.get_geometry_type();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Source, geometry_type, SourceGeometryType);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Source l_Handle = p_Handle.get_id();
            l_Handle.set_geometry_type(*(SourceGeometryType *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Source l_Handle = p_Handle.get_id();
            *((SourceGeometryType *)p_Data) =
                l_Handle.get_geometry_type();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: geometry_type
        }
        {
          // Property: area_type
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(area_type);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(Source::Data, area_type);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
          l_PropertyInfo.handleType =
              AreaTypeEnumHelper::get_enum_id();
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Source l_Handle = p_Handle.get_id();
            l_Handle.get_area_type();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Source,
                                              area_type, AreaType);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Source l_Handle = p_Handle.get_id();
            l_Handle.set_area_type(*(AreaType *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Source l_Handle = p_Handle.get_id();
            *((AreaType *)p_Data) = l_Handle.get_area_type();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: area_type
        }
        {
          // Property: agent_mask
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(agent_mask);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(Source::Data, agent_mask);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Source l_Handle = p_Handle.get_id();
            l_Handle.get_agent_mask();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Source,
                                              agent_mask, uint32_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Source l_Handle = p_Handle.get_id();
            l_Handle.set_agent_mask(*(uint32_t *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Source l_Handle = p_Handle.get_id();
            *((uint32_t *)p_Data) = l_Handle.get_agent_mask();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: agent_mask
        }
        {
          // Property: include_children
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(include_children);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(Source::Data, include_children);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Source l_Handle = p_Handle.get_id();
            l_Handle.is_include_children();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Source,
                                              include_children, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Source l_Handle = p_Handle.get_id();
            l_Handle.set_include_children(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Source l_Handle = p_Handle.get_id();
            *((bool *)p_Data) = l_Handle.is_include_children();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: include_children
        }
        {
          // Property: tile_dirty
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(tile_dirty);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Source::Data, tile_dirty);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Source l_Handle = p_Handle.get_id();
            l_Handle.is_tile_dirty();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Source,
                                              tile_dirty, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Source l_Handle = p_Handle.get_id();
            l_Handle.set_tile_dirty(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Source l_Handle = p_Handle.get_id();
            *((bool *)p_Data) = l_Handle.is_tile_dirty();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: tile_dirty
        }
        {
          // Property: bounds
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(bounds);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Source::Data, bounds);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Source l_Handle = p_Handle.get_id();
            l_Handle.get_bounds();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Source, bounds, Low::Math::Bounds);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Source l_Handle = p_Handle.get_id();
            l_Handle.set_bounds(*(Low::Math::Bounds *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Source l_Handle = p_Handle.get_id();
            *((Low::Math::Bounds *)p_Data) = l_Handle.get_bounds();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: bounds
        }
        {
          // Property: bounds_valid
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(bounds_valid);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Source::Data, bounds_valid);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Source l_Handle = p_Handle.get_id();
            l_Handle.is_bounds_valid();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Source,
                                              bounds_valid, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Source l_Handle = p_Handle.get_id();
            l_Handle.set_bounds_valid(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Source l_Handle = p_Handle.get_id();
            *((bool *)p_Data) = l_Handle.is_bounds_valid();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: bounds_valid
        }
        {
          // Property: entity
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Source::Data, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Source l_Handle = p_Handle.get_id();
            l_Handle.get_entity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Source, entity, Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Source l_Handle = p_Handle.get_id();
            l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Source l_Handle = p_Handle.get_id();
            *((Low::Core::Entity *)p_Data) = l_Handle.get_entity();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: entity
        }
        {
          // Property: unique_id
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Source::Data, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Source l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Source, unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Source l_Handle = p_Handle.get_id();
            *((Low::Util::UniqueId *)p_Data) =
                l_Handle.get_unique_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: unique_id
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void Source::cleanup()
      {
        Low::Util::List<Source> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle Source::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Source Source::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Source l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Source::ms_TypeId;

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

      Source Source::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Source l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Source::ms_TypeId;

        return l_Handle;
      }

      bool Source::is_alive() const
      {
        if (m_Data.m_Type != Source::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == Source::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Source::get_capacity()
      {
        return ms_Capacity;
      }

      Source Source::duplicate(Low::Core::Entity p_Entity) const
      {
        _LOW_ASSERT(is_alive());

        Source l_Handle = make(p_Entity);
        l_Handle.set_mode(get_mode());
        l_Handle.set_geometry_type(get_geometry_type());
        l_Handle.set_area_type(get_area_type());
        l_Handle.set_agent_mask(get_agent_mask());
        l_Handle.set_include_children(is_include_children());
        l_Handle.set_tile_dirty(is_tile_dirty());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      Source Source::duplicate(Source p_Handle,
                               Low::Core::Entity p_Entity)
      {
        return p_Handle.duplicate(p_Entity);
      }

      Low::Util::Handle Source::_duplicate(Low::Util::Handle p_Handle,
                                           Low::Util::Handle p_Entity)
      {
        Source l_Source = p_Handle.get_id();
        Low::Core::Entity l_Entity = p_Entity.get_id();
        return l_Source.duplicate(l_Entity);
      }

      void Source::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        Low::Util::Serial::serialize_enum(
            p_Node["mode"], SourceModeEnumHelper::get_enum_id(),
            static_cast<uint8_t>(get_mode()));
        Low::Util::Serial::serialize_enum(
            p_Node["geometry_type"],
            SourceGeometryTypeEnumHelper::get_enum_id(),
            static_cast<uint8_t>(get_geometry_type()));
        Low::Util::Serial::serialize_enum(
            p_Node["area_type"], AreaTypeEnumHelper::get_enum_id(),
            static_cast<uint8_t>(get_area_type()));
        p_Node["agent_mask"] = get_agent_mask();
        p_Node["include_children"] = is_include_children();
        p_Node["_unique_id"] = Low::Util::U64Id{get_unique_id()};

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Source::serialize(Low::Util::Handle p_Handle,
                             Low::Util::Serial::Node &p_Node)
      {
        Source l_Source = p_Handle.get_id();
        l_Source.serialize(p_Node);
      }

      Low::Util::Handle
      Source::deserialize(Low::Util::Serial::Node &p_Node,
                          Low::Util::Handle p_Creator)
      {
        Low::Util::UniqueId l_HandleUniqueId = 0ull;
        if (p_Node["unique_id"]) {
          l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
        } else if (p_Node["_unique_id"]) {
          l_HandleUniqueId = Low::Util::string_to_hash(
              p_Node["_unique_id"].as<Low::Util::String>());
        }

        Source l_Handle =
            Source::make(p_Creator.get_id(), l_HandleUniqueId);

        if (p_Node["mode"]) {
          l_Handle.set_mode(static_cast<SourceMode>(
              Low::Util::Serial::deserialize_enum(p_Node["mode"])));
        }
        if (p_Node["geometry_type"]) {
          l_Handle.set_geometry_type(static_cast<SourceGeometryType>(
              Low::Util::Serial::deserialize_enum(
                  p_Node["geometry_type"])));
        }
        if (p_Node["area_type"]) {
          l_Handle.set_area_type(static_cast<AreaType>(
              Low::Util::Serial::deserialize_enum(
                  p_Node["area_type"])));
        }
        if (p_Node["agent_mask"]) {
          l_Handle.set_agent_mask(
              p_Node["agent_mask"].as<uint32_t>());
        }
        if (p_Node["include_children"]) {
          l_Handle.set_include_children(
              p_Node["include_children"].as<bool>());
        }
        if (p_Node["unique_id"]) {
          l_Handle.set_unique_id(
              p_Node["unique_id"].as<Low::Util::UniqueId>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void
      Source::broadcast_observable(Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 Source::observe(Low::Util::Name p_Observable,
                          Low::Util::Function<void(Low::Util::Handle,
                                                   Low::Util::Name)>
                              p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Source::observe(Low::Util::Name p_Observable,
                          Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Source::notify(Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        (void)p_Observed;
        if (p_Observable == N(dirty) ||
            p_Observable == N(world_dirty)) {
          mark_dirty();
        }
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Source::_notify(Low::Util::Handle p_Observer,
                           Low::Util::Handle p_Observed,
                           Low::Util::Name p_Observable)
      {
        Source l_Source = p_Observer.get_id();
        l_Source.notify(p_Observed, p_Observable);
      }

      SourceMode Source::get_mode() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mode
        // LOW_CODEGEN::END::CUSTOM:GETTER_mode

        return TYPE_SOA(Source, mode, SourceMode);
      }
      void Source::set_mode(SourceMode p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mode
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_mode

        if (get_mode() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(Source, mode, SourceMode) = p_Value;
          {
            Low::Core::Entity l_Entity = get_entity();
            if (l_Entity.has_component(
                    Low::Core::Component::PrefabInstance::
                        type_id())) {
              Low::Core::Component::PrefabInstance l_Instance =
                  l_Entity.get_component(
                      Low::Core::Component::PrefabInstance::
                          type_id());
              Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
              if (l_Prefab.is_alive()) {
                l_Instance.override(
                    ms_TypeId, N(mode),
                    !l_Prefab.compare_property(*this, N(mode)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mode
          // LOW_CODEGEN::END::CUSTOM:SETTER_mode

          broadcast_observable(N(mode));
        }
      }

      SourceGeometryType Source::get_geometry_type() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_geometry_type
        // LOW_CODEGEN::END::CUSTOM:GETTER_geometry_type

        return TYPE_SOA(Source, geometry_type, SourceGeometryType);
      }
      void Source::set_geometry_type(SourceGeometryType p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_geometry_type
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_geometry_type

        if (get_geometry_type() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(Source, geometry_type, SourceGeometryType) =
              p_Value;
          {
            Low::Core::Entity l_Entity = get_entity();
            if (l_Entity.has_component(
                    Low::Core::Component::PrefabInstance::
                        type_id())) {
              Low::Core::Component::PrefabInstance l_Instance =
                  l_Entity.get_component(
                      Low::Core::Component::PrefabInstance::
                          type_id());
              Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
              if (l_Prefab.is_alive()) {
                l_Instance.override(ms_TypeId, N(geometry_type),
                                    !l_Prefab.compare_property(
                                        *this, N(geometry_type)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_geometry_type
          // LOW_CODEGEN::END::CUSTOM:SETTER_geometry_type

          broadcast_observable(N(geometry_type));
        }
      }

      AreaType Source::get_area_type() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_area_type
        // LOW_CODEGEN::END::CUSTOM:GETTER_area_type

        return TYPE_SOA(Source, area_type, AreaType);
      }
      void Source::set_area_type(AreaType p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_area_type
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_area_type

        if (get_area_type() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(Source, area_type, AreaType) = p_Value;
          {
            Low::Core::Entity l_Entity = get_entity();
            if (l_Entity.has_component(
                    Low::Core::Component::PrefabInstance::
                        type_id())) {
              Low::Core::Component::PrefabInstance l_Instance =
                  l_Entity.get_component(
                      Low::Core::Component::PrefabInstance::
                          type_id());
              Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
              if (l_Prefab.is_alive()) {
                l_Instance.override(
                    ms_TypeId, N(area_type),
                    !l_Prefab.compare_property(*this, N(area_type)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_area_type
          // LOW_CODEGEN::END::CUSTOM:SETTER_area_type

          broadcast_observable(N(area_type));
        }
      }

      uint32_t Source::get_agent_mask() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_agent_mask
        // LOW_CODEGEN::END::CUSTOM:GETTER_agent_mask

        return TYPE_SOA(Source, agent_mask, uint32_t);
      }
      void Source::set_agent_mask(uint32_t p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_agent_mask
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_agent_mask

        if (get_agent_mask() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(Source, agent_mask, uint32_t) = p_Value;
          {
            Low::Core::Entity l_Entity = get_entity();
            if (l_Entity.has_component(
                    Low::Core::Component::PrefabInstance::
                        type_id())) {
              Low::Core::Component::PrefabInstance l_Instance =
                  l_Entity.get_component(
                      Low::Core::Component::PrefabInstance::
                          type_id());
              Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
              if (l_Prefab.is_alive()) {
                l_Instance.override(
                    ms_TypeId, N(agent_mask),
                    !l_Prefab.compare_property(*this, N(agent_mask)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_agent_mask
          // LOW_CODEGEN::END::CUSTOM:SETTER_agent_mask

          broadcast_observable(N(agent_mask));
        }
      }

      bool Source::is_include_children() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_include_children
        // LOW_CODEGEN::END::CUSTOM:GETTER_include_children

        return TYPE_SOA(Source, include_children, bool);
      }
      void Source::toggle_include_children()
      {
        set_include_children(!is_include_children());
      }

      void Source::set_include_children(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_include_children
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_include_children

        if (is_include_children() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(Source, include_children, bool) = p_Value;
          {
            Low::Core::Entity l_Entity = get_entity();
            if (l_Entity.has_component(
                    Low::Core::Component::PrefabInstance::
                        type_id())) {
              Low::Core::Component::PrefabInstance l_Instance =
                  l_Entity.get_component(
                      Low::Core::Component::PrefabInstance::
                          type_id());
              Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
              if (l_Prefab.is_alive()) {
                l_Instance.override(ms_TypeId, N(include_children),
                                    !l_Prefab.compare_property(
                                        *this, N(include_children)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_include_children
          // LOW_CODEGEN::END::CUSTOM:SETTER_include_children

          broadcast_observable(N(include_children));
        }
      }

      bool Source::is_tile_dirty() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_tile_dirty
        // LOW_CODEGEN::END::CUSTOM:GETTER_tile_dirty

        return TYPE_SOA(Source, tile_dirty, bool);
      }
      void Source::toggle_tile_dirty()
      {
        set_tile_dirty(!is_tile_dirty());
      }

      void Source::set_tile_dirty(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_tile_dirty
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_tile_dirty

        // Set new value
        TYPE_SOA(Source, tile_dirty, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_tile_dirty
        // LOW_CODEGEN::END::CUSTOM:SETTER_tile_dirty

        broadcast_observable(N(tile_dirty));
      }

      Low::Math::Bounds &Source::get_bounds() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_bounds
        // LOW_CODEGEN::END::CUSTOM:GETTER_bounds

        return TYPE_SOA(Source, bounds, Low::Math::Bounds);
      }
      void Source::set_bounds(Low::Math::Bounds &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_bounds
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_bounds

        // Set new value
        TYPE_SOA(Source, bounds, Low::Math::Bounds) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_bounds
        // LOW_CODEGEN::END::CUSTOM:SETTER_bounds

        broadcast_observable(N(bounds));
      }

      bool Source::is_bounds_valid() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_bounds_valid
        // LOW_CODEGEN::END::CUSTOM:GETTER_bounds_valid

        return TYPE_SOA(Source, bounds_valid, bool);
      }
      void Source::toggle_bounds_valid()
      {
        set_bounds_valid(!is_bounds_valid());
      }

      void Source::set_bounds_valid(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_bounds_valid
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_bounds_valid

        // Set new value
        TYPE_SOA(Source, bounds_valid, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_bounds_valid
        // LOW_CODEGEN::END::CUSTOM:SETTER_bounds_valid

        broadcast_observable(N(bounds_valid));
      }

      Low::Core::Entity Source::get_entity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity
        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        return TYPE_SOA(Source, entity, Low::Core::Entity);
      }
      void Source::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

        // Set new value
        TYPE_SOA(Source, entity, Low::Core::Entity) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity
        // LOW_CODEGEN::END::CUSTOM:SETTER_entity

        broadcast_observable(N(entity));
      }

      Low::Util::UniqueId Source::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(Source, unique_id, Low::Util::UniqueId);
      }
      void Source::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(Source, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

        broadcast_observable(N(unique_id));
      }

      void Source::mark_dirty()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
        Low::Core::Navigation::mark_source_dirty(*this);
        // LOW_CODEGEN::END::CUSTOM:MARK_dirty
      }

      uint32_t Source::create_instance(u32 &p_PageIndex,
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

      u32 Source::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Source.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Source::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Source::get_page_for_index(const u32 p_Index,
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

    } // namespace Navigation
  } // namespace Core
} // namespace Low
