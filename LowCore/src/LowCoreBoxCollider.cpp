#include "LowCoreBoxCollider.h"

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
#include "LowCoreRigidbody.h"
#include "LowCoreTransform.h"
#include "LowCoreRegion.h"
#include "LowCoreScene.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      Low::Util::Set<Low::Core::Component::BoxCollider>
          Low::Core::Component::BoxCollider::ms_Dirty;

      static Low::Core::Physics::World
      get_physics_world(Low::Core::Entity p_Entity)
      {
        _LOW_ASSERT(p_Entity.is_alive());
        Low::Core::Region l_Region = p_Entity.get_region();
        _LOW_ASSERT(l_Region.is_alive());
        Low::Core::Scene l_Scene = l_Region.get_scene();
        _LOW_ASSERT(l_Scene.is_alive());
        Low::Core::Physics::World l_World =
            l_Scene.get_physics_world();
        _LOW_ASSERT(l_World.is_alive());
        return l_World;
      }

      static Low::Math::Vector3
      get_world_collider_position(Low::Core::Entity p_Entity,
                                  Low::Math::Vector3 p_Center)
      {
        Low::Core::Component::Transform l_Transform =
            p_Entity.get_transform();
        _LOW_ASSERT(l_Transform.is_alive());
        return l_Transform.get_world_position() +
               (l_Transform.get_world_rotation() * p_Center);
      }
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 BoxCollider::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          BoxCollider::IDENTIFIER(LOW_NAME(1181529166),
                                  LOW_NAME(3630028570));
      uint32_t BoxCollider::ms_Capacity = 0u;
      uint32_t BoxCollider::ms_PageSize = 0u;
      Low::Util::List<BoxCollider> BoxCollider::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          BoxCollider::ms_Pages;

      Low::Util::Handle BoxCollider::_make(Low::Util::Handle p_Entity)
      {
        Low::Core::Entity l_Entity = p_Entity.get_id();
        LOW_ASSERT(l_Entity.is_alive(),
                   "Cannot create component for dead entity");
        return make(l_Entity).get_id();
      }

      BoxCollider BoxCollider::make(Low::Core::Entity p_Entity)
      {
        return make(p_Entity, 0ull);
      }

      BoxCollider BoxCollider::make(Low::Core::Entity p_Entity,
                                    Low::Util::UniqueId p_UniqueId)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        BoxCollider l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = BoxCollider::ms_TypeId;

        ACCESSOR_TYPE_SOA(l_Handle, BoxCollider, trigger, bool) =
            false;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, BoxCollider, shape,
                                   Low::Core::Physics::Shape))
            Low::Core::Physics::Shape();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, BoxCollider, static_body,
                                   Low::Core::Physics::Body))
            Low::Core::Physics::Body();
        ACCESSOR_TYPE_SOA(l_Handle, BoxCollider, initialized, bool) =
            false;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, BoxCollider, entity,
                                   Low::Core::Entity))
            Low::Core::Entity();
        ACCESSOR_TYPE_SOA(l_Handle, BoxCollider, dirty, bool) = false;

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
        ACCESSOR_TYPE_SOA(l_Handle, BoxCollider, center,
                          Low::Math::Vector3) =
            Low::Math::Vector3(0.0f);
        ACCESSOR_TYPE_SOA(l_Handle, BoxCollider, half_extents,
                          Low::Math::Vector3) =
            Low::Math::Vector3(0.5f);

        l_Handle.mark_dirty();

        {
          Low::Core::Component::Transform l_Transform =
              p_Entity.get_transform();
          if (l_Transform.is_alive()) {
            l_Transform.observe(N(world_scale_changed),
                                l_Handle.get_id());
          }
        }
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void BoxCollider::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          if (get_static_body().is_alive()) {
            get_static_body().destroy();
          }
          if (get_shape().is_alive()) {
            get_shape().destroy();
          }
          set_initialized(false);
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

      void BoxCollider::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowCore), N(BoxCollider));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowCore),
                                                      N(BoxCollider));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, BoxCollider::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(BoxCollider);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &BoxCollider::is_alive;
        l_TypeInfo.destroy = &BoxCollider::destroy;
        l_TypeInfo.serialize = &BoxCollider::serialize;
        l_TypeInfo.deserialize = &BoxCollider::deserialize;
        l_TypeInfo.find_by_index = &BoxCollider::_find_by_index;
        l_TypeInfo.notify = &BoxCollider::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.make_default = nullptr;
        l_TypeInfo.make_component = &BoxCollider::_make;
        l_TypeInfo.duplicate_default = nullptr;
        l_TypeInfo.duplicate_component = &BoxCollider::_duplicate;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &BoxCollider::living_instances);
        l_TypeInfo.get_living_count = &BoxCollider::living_count;
        l_TypeInfo.component = true;
        l_TypeInfo.uiComponent = false;
        {
          // Property: center
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(center);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(BoxCollider::Data, center);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            BoxCollider l_Handle = p_Handle.get_id();
            l_Handle.get_center();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, BoxCollider, center, Low::Math::Vector3);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            BoxCollider l_Handle = p_Handle.get_id();
            l_Handle.set_center(*(Low::Math::Vector3 *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            BoxCollider l_Handle = p_Handle.get_id();
            *((Low::Math::Vector3 *)p_Data) = l_Handle.get_center();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: center
        }
        {
          // Property: half_extents
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(half_extents);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(BoxCollider::Data, half_extents);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            BoxCollider l_Handle = p_Handle.get_id();
            l_Handle.get_half_extents();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, BoxCollider,
                                              half_extents,
                                              Low::Math::Vector3);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            BoxCollider l_Handle = p_Handle.get_id();
            l_Handle.set_half_extents(*(Low::Math::Vector3 *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            BoxCollider l_Handle = p_Handle.get_id();
            *((Low::Math::Vector3 *)p_Data) =
                l_Handle.get_half_extents();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: half_extents
        }
        {
          // Property: trigger
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(trigger);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(BoxCollider::Data, trigger);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            BoxCollider l_Handle = p_Handle.get_id();
            l_Handle.is_trigger();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, BoxCollider,
                                              trigger, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            BoxCollider l_Handle = p_Handle.get_id();
            l_Handle.set_trigger(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            BoxCollider l_Handle = p_Handle.get_id();
            *((bool *)p_Data) = l_Handle.is_trigger();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: trigger
        }
        {
          // Property: shape
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(shape);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(BoxCollider::Data, shape);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Core::Physics::Shape::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            BoxCollider l_Handle = p_Handle.get_id();
            l_Handle.get_shape();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, BoxCollider, shape,
                Low::Core::Physics::Shape);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            BoxCollider l_Handle = p_Handle.get_id();
            *((Low::Core::Physics::Shape *)p_Data) =
                l_Handle.get_shape();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: shape
        }
        {
          // Property: static_body
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(static_body);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(BoxCollider::Data, static_body);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Core::Physics::Body::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            BoxCollider l_Handle = p_Handle.get_id();
            l_Handle.get_static_body();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, BoxCollider, static_body,
                Low::Core::Physics::Body);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            BoxCollider l_Handle = p_Handle.get_id();
            *((Low::Core::Physics::Body *)p_Data) =
                l_Handle.get_static_body();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: static_body
        }
        {
          // Property: initialized
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(initialized);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(BoxCollider::Data, initialized);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            BoxCollider l_Handle = p_Handle.get_id();
            l_Handle.is_initialized();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, BoxCollider,
                                              initialized, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            BoxCollider l_Handle = p_Handle.get_id();
            *((bool *)p_Data) = l_Handle.is_initialized();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: initialized
        }
        {
          // Property: entity
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(BoxCollider::Data, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            BoxCollider l_Handle = p_Handle.get_id();
            l_Handle.get_entity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, BoxCollider, entity, Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            BoxCollider l_Handle = p_Handle.get_id();
            l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            BoxCollider l_Handle = p_Handle.get_id();
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
              offsetof(BoxCollider::Data, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            BoxCollider l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, BoxCollider,
                                              unique_id,
                                              Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            BoxCollider l_Handle = p_Handle.get_id();
            *((Low::Util::UniqueId *)p_Data) =
                l_Handle.get_unique_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: unique_id
        }
        {
          // Property: dirty
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(dirty);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(BoxCollider::Data, dirty);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            BoxCollider l_Handle = p_Handle.get_id();
            l_Handle.is_dirty();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, BoxCollider,
                                              dirty, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            BoxCollider l_Handle = p_Handle.get_id();
            l_Handle.set_dirty(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            BoxCollider l_Handle = p_Handle.get_id();
            *((bool *)p_Data) = l_Handle.is_dirty();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: dirty
        }
        {
          // Function: rebuild
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(rebuild);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: rebuild
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void BoxCollider::cleanup()
      {
        Low::Util::List<BoxCollider> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle BoxCollider::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      BoxCollider BoxCollider::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        BoxCollider l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = BoxCollider::ms_TypeId;

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

      BoxCollider BoxCollider::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        BoxCollider l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = BoxCollider::ms_TypeId;

        return l_Handle;
      }

      bool BoxCollider::is_alive() const
      {
        if (m_Data.m_Type != BoxCollider::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == BoxCollider::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t BoxCollider::get_capacity()
      {
        return ms_Capacity;
      }

      BoxCollider
      BoxCollider::duplicate(Low::Core::Entity p_Entity) const
      {
        _LOW_ASSERT(is_alive());

        BoxCollider l_Handle = make(p_Entity);
        l_Handle.set_center(get_center());
        l_Handle.set_half_extents(get_half_extents());
        l_Handle.set_trigger(is_trigger());
        if (get_shape().is_alive()) {
          l_Handle.set_shape(get_shape());
        }
        if (get_static_body().is_alive()) {
          l_Handle.set_static_body(get_static_body());
        }
        l_Handle.set_initialized(is_initialized());
        l_Handle.set_dirty(is_dirty());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      BoxCollider BoxCollider::duplicate(BoxCollider p_Handle,
                                         Low::Core::Entity p_Entity)
      {
        return p_Handle.duplicate(p_Entity);
      }

      Low::Util::Handle
      BoxCollider::_duplicate(Low::Util::Handle p_Handle,
                              Low::Util::Handle p_Entity)
      {
        BoxCollider l_BoxCollider = p_Handle.get_id();
        Low::Core::Entity l_Entity = p_Entity.get_id();
        return l_BoxCollider.duplicate(l_Entity);
      }

      void
      BoxCollider::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["center"] = get_center();
        p_Node["half_extents"] = get_half_extents();
        p_Node["trigger"] = is_trigger();
        p_Node["_unique_id"] = Low::Util::U64Id{get_unique_id()};

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void BoxCollider::serialize(Low::Util::Handle p_Handle,
                                  Low::Util::Serial::Node &p_Node)
      {
        BoxCollider l_BoxCollider = p_Handle.get_id();
        l_BoxCollider.serialize(p_Node);
      }

      Low::Util::Handle
      BoxCollider::deserialize(Low::Util::Serial::Node &p_Node,
                               Low::Util::Handle p_Creator)
      {
        Low::Util::UniqueId l_HandleUniqueId = 0ull;
        if (p_Node["unique_id"]) {
          l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
        } else if (p_Node["_unique_id"]) {
          l_HandleUniqueId = Low::Util::string_to_hash(
              p_Node["_unique_id"].as<Low::Util::String>());
        }

        BoxCollider l_Handle =
            BoxCollider::make(p_Creator.get_id(), l_HandleUniqueId);

        if (p_Node["center"]) {
          l_Handle.set_center(
              p_Node["center"].as<Low::Math::Vector3>());
        }
        if (p_Node["half_extents"]) {
          l_Handle.set_half_extents(
              p_Node["half_extents"].as<Low::Math::Vector3>());
        }
        if (p_Node["trigger"]) {
          l_Handle.set_trigger(p_Node["trigger"].as<bool>());
        }
        if (p_Node["unique_id"]) {
          l_Handle.set_unique_id(
              p_Node["unique_id"].as<Low::Util::UniqueId>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void BoxCollider::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64
      BoxCollider::observe(Low::Util::Name p_Observable,
                           Low::Util::Function<void(Low::Util::Handle,
                                                    Low::Util::Name)>
                               p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 BoxCollider::observe(Low::Util::Name p_Observable,
                               Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void BoxCollider::notify(Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        if (p_Observable == N(world_scale_changed)) {
          mark_dirty();
        }
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void BoxCollider::_notify(Low::Util::Handle p_Observer,
                                Low::Util::Handle p_Observed,
                                Low::Util::Name p_Observable)
      {
        BoxCollider l_BoxCollider = p_Observer.get_id();
        l_BoxCollider.notify(p_Observed, p_Observable);
      }

      Low::Math::Vector3 BoxCollider::get_center() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_center
        // LOW_CODEGEN::END::CUSTOM:GETTER_center

        return TYPE_SOA(BoxCollider, center, Low::Math::Vector3);
      }
      void BoxCollider::set_center(float p_X, float p_Y, float p_Z)
      {
        Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
        set_center(p_Val);
      }

      void BoxCollider::set_center_x(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_center();
        l_Value.x = p_Value;
        set_center(l_Value);
      }

      void BoxCollider::set_center_y(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_center();
        l_Value.y = p_Value;
        set_center(l_Value);
      }

      void BoxCollider::set_center_z(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_center();
        l_Value.z = p_Value;
        set_center(l_Value);
      }

      void BoxCollider::set_center(Low::Math::Vector3 p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_center
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_center

        if (get_center() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(BoxCollider, center, Low::Math::Vector3) = p_Value;
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
                    ms_TypeId, N(center),
                    !l_Prefab.compare_property(*this, N(center)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_center
          // LOW_CODEGEN::END::CUSTOM:SETTER_center

          broadcast_observable(N(center));
        }
      }

      Low::Math::Vector3 BoxCollider::get_half_extents() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_half_extents
        // LOW_CODEGEN::END::CUSTOM:GETTER_half_extents

        return TYPE_SOA(BoxCollider, half_extents,
                        Low::Math::Vector3);
      }
      void BoxCollider::set_half_extents(float p_X, float p_Y,
                                         float p_Z)
      {
        Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
        set_half_extents(p_Val);
      }

      void BoxCollider::set_half_extents_x(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_half_extents();
        l_Value.x = p_Value;
        set_half_extents(l_Value);
      }

      void BoxCollider::set_half_extents_y(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_half_extents();
        l_Value.y = p_Value;
        set_half_extents(l_Value);
      }

      void BoxCollider::set_half_extents_z(float p_Value)
      {
        Low::Math::Vector3 l_Value = get_half_extents();
        l_Value.z = p_Value;
        set_half_extents(l_Value);
      }

      void BoxCollider::set_half_extents(Low::Math::Vector3 p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_half_extents
        p_Value = glm::max(p_Value, Low::Math::Vector3(0.001f));
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_half_extents

        if (get_half_extents() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(BoxCollider, half_extents, Low::Math::Vector3) =
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
                l_Instance.override(ms_TypeId, N(half_extents),
                                    !l_Prefab.compare_property(
                                        *this, N(half_extents)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_half_extents
          // LOW_CODEGEN::END::CUSTOM:SETTER_half_extents

          broadcast_observable(N(half_extents));
        }
      }

      bool BoxCollider::is_trigger() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_trigger
        // LOW_CODEGEN::END::CUSTOM:GETTER_trigger

        return TYPE_SOA(BoxCollider, trigger, bool);
      }
      void BoxCollider::toggle_trigger()
      {
        set_trigger(!is_trigger());
      }

      void BoxCollider::set_trigger(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_trigger
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_trigger

        // Set new value
        TYPE_SOA(BoxCollider, trigger, bool) = p_Value;
        {
          Low::Core::Entity l_Entity = get_entity();
          if (l_Entity.has_component(
                  Low::Core::Component::PrefabInstance::type_id())) {
            Low::Core::Component::PrefabInstance l_Instance =
                l_Entity.get_component(
                    Low::Core::Component::PrefabInstance::type_id());
            Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
            if (l_Prefab.is_alive()) {
              l_Instance.override(
                  ms_TypeId, N(trigger),
                  !l_Prefab.compare_property(*this, N(trigger)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_trigger
        // LOW_CODEGEN::END::CUSTOM:SETTER_trigger

        broadcast_observable(N(trigger));
      }

      Low::Core::Physics::Shape BoxCollider::get_shape() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_shape
        // LOW_CODEGEN::END::CUSTOM:GETTER_shape

        return TYPE_SOA(BoxCollider, shape,
                        Low::Core::Physics::Shape);
      }
      void BoxCollider::set_shape(Low::Core::Physics::Shape p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_shape
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_shape

        // Set new value
        TYPE_SOA(BoxCollider, shape, Low::Core::Physics::Shape) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_shape
        // LOW_CODEGEN::END::CUSTOM:SETTER_shape

        broadcast_observable(N(shape));
      }

      Low::Core::Physics::Body BoxCollider::get_static_body() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_static_body
        // LOW_CODEGEN::END::CUSTOM:GETTER_static_body

        return TYPE_SOA(BoxCollider, static_body,
                        Low::Core::Physics::Body);
      }
      void
      BoxCollider::set_static_body(Low::Core::Physics::Body p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_static_body
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_static_body

        // Set new value
        TYPE_SOA(BoxCollider, static_body, Low::Core::Physics::Body) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_static_body
        // LOW_CODEGEN::END::CUSTOM:SETTER_static_body

        broadcast_observable(N(static_body));
      }

      bool BoxCollider::is_initialized() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:GETTER_initialized

        return TYPE_SOA(BoxCollider, initialized, bool);
      }
      void BoxCollider::toggle_initialized()
      {
        set_initialized(!is_initialized());
      }

      void BoxCollider::set_initialized(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_initialized

        // Set new value
        TYPE_SOA(BoxCollider, initialized, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:SETTER_initialized

        broadcast_observable(N(initialized));
      }

      Low::Core::Entity BoxCollider::get_entity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity
        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        return TYPE_SOA(BoxCollider, entity, Low::Core::Entity);
      }
      void BoxCollider::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

        // Set new value
        TYPE_SOA(BoxCollider, entity, Low::Core::Entity) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity
        // LOW_CODEGEN::END::CUSTOM:SETTER_entity

        broadcast_observable(N(entity));
      }

      Low::Util::UniqueId BoxCollider::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(BoxCollider, unique_id, Low::Util::UniqueId);
      }
      void BoxCollider::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(BoxCollider, unique_id, Low::Util::UniqueId) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

        broadcast_observable(N(unique_id));
      }

      bool BoxCollider::is_dirty() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

        return TYPE_SOA(BoxCollider, dirty, bool);
      }
      void BoxCollider::toggle_dirty()
      {
        set_dirty(!is_dirty());
      }

      void BoxCollider::set_dirty(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

        // Set new value
        TYPE_SOA(BoxCollider, dirty, bool) = p_Value;

        if (p_Value) {
          mark_dirty();
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:SETTER_dirty

        broadcast_observable(N(dirty));
      }

      void BoxCollider::mark_dirty()
      {
        if (!is_dirty()) {
          TYPE_SOA(BoxCollider, dirty, bool) = true;
          // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
          ms_Dirty.insert(get_id());
          // LOW_CODEGEN::END::CUSTOM:MARK_dirty
        }
      }

      void BoxCollider::rebuild()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_rebuild
        _LOW_ASSERT(is_alive());

        if (get_static_body().is_alive()) {
          get_static_body().destroy();
          set_static_body(Low::Core::Physics::Body());
        }
        if (get_shape().is_alive()) {
          get_shape().destroy();
          set_shape(Low::Core::Physics::Shape());
        }

        Low::Core::Entity l_Entity = get_entity();
        Low::Core::Component::Transform l_Transform =
            l_Entity.get_transform();
        Low::Math::Vector3 l_WorldScale =
            l_Transform.get_world_scale();

        Low::Core::Physics::World l_World =
            get_physics_world(l_Entity);

        Low::Math::Shape l_Shape;
        l_Shape.type = Low::Math::ShapeType::BOX;
        l_Shape.box.position = get_center();
        l_Shape.box.rotation =
            Low::Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
        l_Shape.box.halfExtents = get_half_extents() * l_WorldScale;

        Low::Core::Physics::Shape l_PhysicsShape =
            Low::Core::Physics::Shape::make(l_World, l_Shape);
        set_shape(l_PhysicsShape);

        if (l_Entity.has_component(Rigidbody::type_id())) {
          Rigidbody l_Rigidbody =
              l_Entity.get_component(Rigidbody::type_id());
          l_Rigidbody.rebuild();
        } else {
          _LOW_ASSERT(l_Transform.is_alive());

          Low::Core::Physics::Body l_Body =
              Low::Core::Physics::Body::make(
                  l_World, l_PhysicsShape,
                  get_world_collider_position(l_Entity, get_center()),
                  l_Transform.get_world_rotation(),
                  Low::Core::Physics::BodyMotionType::STATIC, 1.0f,
                  false);
          set_static_body(l_Body);
        }

        set_initialized(true);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_rebuild
      }

      uint32_t BoxCollider::create_instance(u32 &p_PageIndex,
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

      u32 BoxCollider::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for BoxCollider.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, BoxCollider::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool BoxCollider::get_page_for_index(const u32 p_Index,
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

    } // namespace Component
  } // namespace Core
} // namespace Low
