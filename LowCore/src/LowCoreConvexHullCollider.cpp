#include "LowCoreConvexHullCollider.h"

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
      Low::Util::Set<Low::Core::Component::ConvexHullCollider>
          Low::Core::Component::ConvexHullCollider::ms_Dirty;

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
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 ConvexHullCollider::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          ConvexHullCollider::IDENTIFIER(LOW_NAME(1181529166),
                                         LOW_NAME(3378801746));
      uint32_t ConvexHullCollider::ms_Capacity = 0u;
      uint32_t ConvexHullCollider::ms_PageSize = 0u;
      Low::Util::List<ConvexHullCollider>
          ConvexHullCollider::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          ConvexHullCollider::ms_Pages;

      Low::Util::Handle
      ConvexHullCollider::_make(Low::Util::Handle p_Entity)
      {
        Low::Core::Entity l_Entity = p_Entity.get_id();
        LOW_ASSERT(l_Entity.is_alive(),
                   "Cannot create component for dead entity");
        return make(l_Entity).get_id();
      }

      ConvexHullCollider
      ConvexHullCollider::make(Low::Core::Entity p_Entity)
      {
        return make(p_Entity, 0ull);
      }

      ConvexHullCollider
      ConvexHullCollider::make(Low::Core::Entity p_Entity,
                               Low::Util::UniqueId p_UniqueId)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        ConvexHullCollider l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = ConvexHullCollider::ms_TypeId;

        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, ConvexHullCollider, points,
            Low::Util::List<Low::Math::Vector3>))
            Low::Util::List<Low::Math::Vector3>();
        ACCESSOR_TYPE_SOA(l_Handle, ConvexHullCollider, trigger,
                          bool) = false;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, ConvexHullCollider,
                                   shape, Low::Core::Physics::Shape))
            Low::Core::Physics::Shape();
        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, ConvexHullCollider, static_body,
            Low::Core::Physics::Body)) Low::Core::Physics::Body();
        ACCESSOR_TYPE_SOA(l_Handle, ConvexHullCollider, initialized,
                          bool) = false;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, ConvexHullCollider,
                                   entity, Low::Core::Entity))
            Low::Core::Entity();
        ACCESSOR_TYPE_SOA(l_Handle, ConvexHullCollider, dirty, bool) =
            false;

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

      void ConvexHullCollider::destroy()
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

      void ConvexHullCollider::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowCore), N(ConvexHullCollider));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(
            N(LowCore), N(ConvexHullCollider));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, ConvexHullCollider::Data::get_size(),
                ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(ConvexHullCollider);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &ConvexHullCollider::is_alive;
        l_TypeInfo.destroy = &ConvexHullCollider::destroy;
        l_TypeInfo.serialize = &ConvexHullCollider::serialize;
        l_TypeInfo.deserialize = &ConvexHullCollider::deserialize;
        l_TypeInfo.find_by_index =
            &ConvexHullCollider::_find_by_index;
        l_TypeInfo.notify = &ConvexHullCollider::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.make_default = nullptr;
        l_TypeInfo.make_component = &ConvexHullCollider::_make;
        l_TypeInfo.duplicate_default = nullptr;
        l_TypeInfo.duplicate_component =
            &ConvexHullCollider::_duplicate;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &ConvexHullCollider::living_instances);
        l_TypeInfo.get_living_count =
            &ConvexHullCollider::living_count;
        l_TypeInfo.component = true;
        l_TypeInfo.uiComponent = false;
        {
          // Property: points
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(points);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(ConvexHullCollider::Data, points);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ConvexHullCollider l_Handle = p_Handle.get_id();
            l_Handle.get_points();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ConvexHullCollider, points,
                Low::Util::List<Low::Math::Vector3>);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ConvexHullCollider l_Handle = p_Handle.get_id();
            l_Handle.set_points(
                *(Low::Util::List<Low::Math::Vector3> *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ConvexHullCollider l_Handle = p_Handle.get_id();
            *((Low::Util::List<Low::Math::Vector3> *)p_Data) =
                l_Handle.get_points();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: points
        }
        {
          // Property: trigger
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(trigger);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(ConvexHullCollider::Data, trigger);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ConvexHullCollider l_Handle = p_Handle.get_id();
            l_Handle.is_trigger();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ConvexHullCollider, trigger, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ConvexHullCollider l_Handle = p_Handle.get_id();
            l_Handle.set_trigger(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ConvexHullCollider l_Handle = p_Handle.get_id();
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
              offsetof(ConvexHullCollider::Data, shape);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Core::Physics::Shape::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ConvexHullCollider l_Handle = p_Handle.get_id();
            l_Handle.get_shape();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ConvexHullCollider, shape,
                Low::Core::Physics::Shape);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ConvexHullCollider l_Handle = p_Handle.get_id();
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
              offsetof(ConvexHullCollider::Data, static_body);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Core::Physics::Body::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ConvexHullCollider l_Handle = p_Handle.get_id();
            l_Handle.get_static_body();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ConvexHullCollider, static_body,
                Low::Core::Physics::Body);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ConvexHullCollider l_Handle = p_Handle.get_id();
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
              offsetof(ConvexHullCollider::Data, initialized);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ConvexHullCollider l_Handle = p_Handle.get_id();
            l_Handle.is_initialized();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ConvexHullCollider, initialized, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ConvexHullCollider l_Handle = p_Handle.get_id();
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
              offsetof(ConvexHullCollider::Data, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ConvexHullCollider l_Handle = p_Handle.get_id();
            l_Handle.get_entity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ConvexHullCollider, entity,
                Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ConvexHullCollider l_Handle = p_Handle.get_id();
            l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ConvexHullCollider l_Handle = p_Handle.get_id();
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
              offsetof(ConvexHullCollider::Data, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ConvexHullCollider l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ConvexHullCollider, unique_id,
                Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ConvexHullCollider l_Handle = p_Handle.get_id();
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
              offsetof(ConvexHullCollider::Data, dirty);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ConvexHullCollider l_Handle = p_Handle.get_id();
            l_Handle.is_dirty();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, ConvexHullCollider, dirty, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ConvexHullCollider l_Handle = p_Handle.get_id();
            l_Handle.set_dirty(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ConvexHullCollider l_Handle = p_Handle.get_id();
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

      void ConvexHullCollider::cleanup()
      {
        Low::Util::List<ConvexHullCollider> l_Instances =
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
      ConvexHullCollider::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      ConvexHullCollider
      ConvexHullCollider::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        ConvexHullCollider l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = ConvexHullCollider::ms_TypeId;

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

      ConvexHullCollider
      ConvexHullCollider::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        ConvexHullCollider l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = ConvexHullCollider::ms_TypeId;

        return l_Handle;
      }

      bool ConvexHullCollider::is_alive() const
      {
        if (m_Data.m_Type != ConvexHullCollider::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == ConvexHullCollider::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t ConvexHullCollider::get_capacity()
      {
        return ms_Capacity;
      }

      ConvexHullCollider
      ConvexHullCollider::duplicate(Low::Core::Entity p_Entity) const
      {
        _LOW_ASSERT(is_alive());

        ConvexHullCollider l_Handle = make(p_Entity);
        l_Handle.set_points(get_points());
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

      ConvexHullCollider
      ConvexHullCollider::duplicate(ConvexHullCollider p_Handle,
                                    Low::Core::Entity p_Entity)
      {
        return p_Handle.duplicate(p_Entity);
      }

      Low::Util::Handle
      ConvexHullCollider::_duplicate(Low::Util::Handle p_Handle,
                                     Low::Util::Handle p_Entity)
      {
        ConvexHullCollider l_ConvexHullCollider = p_Handle.get_id();
        Low::Core::Entity l_Entity = p_Entity.get_id();
        return l_ConvexHullCollider.duplicate(l_Entity);
      }

      void ConvexHullCollider::serialize(
          Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["trigger"] = is_trigger();
        p_Node["_unique_id"] = Low::Util::U64Id{get_unique_id()};

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        for (const Math::Vector3 &i_Point : get_points()) {
          Util::Serial::Node i_PointNode;
          i_PointNode = i_Point;
          p_Node["points"].push_back(i_PointNode);
        }
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void
      ConvexHullCollider::serialize(Low::Util::Handle p_Handle,
                                    Low::Util::Serial::Node &p_Node)
      {
        ConvexHullCollider l_ConvexHullCollider = p_Handle.get_id();
        l_ConvexHullCollider.serialize(p_Node);
      }

      Low::Util::Handle
      ConvexHullCollider::deserialize(Low::Util::Serial::Node &p_Node,
                                      Low::Util::Handle p_Creator)
      {
        Low::Util::UniqueId l_HandleUniqueId = 0ull;
        if (p_Node["unique_id"]) {
          l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
        } else if (p_Node["_unique_id"]) {
          l_HandleUniqueId = Low::Util::string_to_hash(
              p_Node["_unique_id"].as<Low::Util::String>());
        }

        ConvexHullCollider l_Handle = ConvexHullCollider::make(
            p_Creator.get_id(), l_HandleUniqueId);

        if (p_Node["points"]) {
        }
        if (p_Node["trigger"]) {
          l_Handle.set_trigger(p_Node["trigger"].as<bool>());
        }
        if (p_Node["unique_id"]) {
          l_Handle.set_unique_id(
              p_Node["unique_id"].as<Low::Util::UniqueId>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        if (p_Node["points"]) {
          Util::List<Math::Vector3> l_Points;
          if (Util::Serial::Node::Seq *l_PointNodes =
                  p_Node["points"].as_seq()) {
            l_Points.reserve(l_PointNodes->size());
            for (Util::Serial::Node &i_PointNode : *l_PointNodes) {
              l_Points.push_back(i_PointNode.as<Math::Vector3>());
            }
          }
          l_Handle.set_points(l_Points);
        }
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void ConvexHullCollider::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 ConvexHullCollider::observe(
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

      u64
      ConvexHullCollider::observe(Low::Util::Name p_Observable,
                                  Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void ConvexHullCollider::notify(Low::Util::Handle p_Observed,
                                      Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        if (p_Observable == N(world_scale_changed)) {
          mark_dirty();
        }
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void ConvexHullCollider::_notify(Low::Util::Handle p_Observer,
                                       Low::Util::Handle p_Observed,
                                       Low::Util::Name p_Observable)
      {
        ConvexHullCollider l_ConvexHullCollider = p_Observer.get_id();
        l_ConvexHullCollider.notify(p_Observed, p_Observable);
      }

      Low::Util::List<Low::Math::Vector3> &
      ConvexHullCollider::get_points() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_points
        // LOW_CODEGEN::END::CUSTOM:GETTER_points

        return TYPE_SOA(ConvexHullCollider, points,
                        Low::Util::List<Low::Math::Vector3>);
      }
      void ConvexHullCollider::set_points(
          Low::Util::List<Low::Math::Vector3> &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_points
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_points

        // Set new value
        TYPE_SOA(ConvexHullCollider, points,
                 Low::Util::List<Low::Math::Vector3>) = p_Value;
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
                  ms_TypeId, N(points),
                  !l_Prefab.compare_property(*this, N(points)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_points
        // LOW_CODEGEN::END::CUSTOM:SETTER_points

        broadcast_observable(N(points));
      }

      bool ConvexHullCollider::is_trigger() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_trigger
        // LOW_CODEGEN::END::CUSTOM:GETTER_trigger

        return TYPE_SOA(ConvexHullCollider, trigger, bool);
      }
      void ConvexHullCollider::toggle_trigger()
      {
        set_trigger(!is_trigger());
      }

      void ConvexHullCollider::set_trigger(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_trigger
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_trigger

        // Set new value
        TYPE_SOA(ConvexHullCollider, trigger, bool) = p_Value;
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

      Low::Core::Physics::Shape ConvexHullCollider::get_shape() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_shape
        // LOW_CODEGEN::END::CUSTOM:GETTER_shape

        return TYPE_SOA(ConvexHullCollider, shape,
                        Low::Core::Physics::Shape);
      }
      void
      ConvexHullCollider::set_shape(Low::Core::Physics::Shape p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_shape
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_shape

        // Set new value
        TYPE_SOA(ConvexHullCollider, shape,
                 Low::Core::Physics::Shape) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_shape
        // LOW_CODEGEN::END::CUSTOM:SETTER_shape

        broadcast_observable(N(shape));
      }

      Low::Core::Physics::Body
      ConvexHullCollider::get_static_body() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_static_body
        // LOW_CODEGEN::END::CUSTOM:GETTER_static_body

        return TYPE_SOA(ConvexHullCollider, static_body,
                        Low::Core::Physics::Body);
      }
      void ConvexHullCollider::set_static_body(
          Low::Core::Physics::Body p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_static_body
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_static_body

        // Set new value
        TYPE_SOA(ConvexHullCollider, static_body,
                 Low::Core::Physics::Body) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_static_body
        // LOW_CODEGEN::END::CUSTOM:SETTER_static_body

        broadcast_observable(N(static_body));
      }

      bool ConvexHullCollider::is_initialized() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:GETTER_initialized

        return TYPE_SOA(ConvexHullCollider, initialized, bool);
      }
      void ConvexHullCollider::toggle_initialized()
      {
        set_initialized(!is_initialized());
      }

      void ConvexHullCollider::set_initialized(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_initialized

        // Set new value
        TYPE_SOA(ConvexHullCollider, initialized, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_initialized
        // LOW_CODEGEN::END::CUSTOM:SETTER_initialized

        broadcast_observable(N(initialized));
      }

      Low::Core::Entity ConvexHullCollider::get_entity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity
        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        return TYPE_SOA(ConvexHullCollider, entity,
                        Low::Core::Entity);
      }
      void ConvexHullCollider::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

        // Set new value
        TYPE_SOA(ConvexHullCollider, entity, Low::Core::Entity) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity
        // LOW_CODEGEN::END::CUSTOM:SETTER_entity

        broadcast_observable(N(entity));
      }

      Low::Util::UniqueId ConvexHullCollider::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(ConvexHullCollider, unique_id,
                        Low::Util::UniqueId);
      }
      void
      ConvexHullCollider::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(ConvexHullCollider, unique_id, Low::Util::UniqueId) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

        broadcast_observable(N(unique_id));
      }

      bool ConvexHullCollider::is_dirty() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

        return TYPE_SOA(ConvexHullCollider, dirty, bool);
      }
      void ConvexHullCollider::toggle_dirty()
      {
        set_dirty(!is_dirty());
      }

      void ConvexHullCollider::set_dirty(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

        // Set new value
        TYPE_SOA(ConvexHullCollider, dirty, bool) = p_Value;

        if (p_Value) {
          mark_dirty();
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:SETTER_dirty

        broadcast_observable(N(dirty));
      }

      void ConvexHullCollider::mark_dirty()
      {
        if (!is_dirty()) {
          TYPE_SOA(ConvexHullCollider, dirty, bool) = true;
          // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
          ms_Dirty.insert(get_id());
          // LOW_CODEGEN::END::CUSTOM:MARK_dirty
        }
      }

      void ConvexHullCollider::rebuild()
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

        if (get_points().empty()) {
          return;
        }

        if (get_points().size() < 4) {
          return;
        }

        Low::Core::Entity l_Entity = get_entity();
        Low::Core::Component::Transform l_Transform =
            l_Entity.get_transform();
        Low::Math::Vector3 l_WorldScale =
            l_Transform.get_world_scale();

        Low::Core::Physics::World l_World =
            get_physics_world(l_Entity);

        Low::Util::List<Low::Math::Vector3> l_ScaledPoints;
        l_ScaledPoints.reserve(get_points().size());
        for (const Low::Math::Vector3 &i_Point : get_points()) {
          l_ScaledPoints.push_back(i_Point * l_WorldScale);
        }

        Low::Core::Physics::Shape l_PhysicsShape =
            Low::Core::Physics::Shape::make_convex_hull(
                l_World, l_ScaledPoints);
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
                  l_Transform.get_world_position(),
                  l_Transform.get_world_rotation(),
                  Low::Core::Physics::BodyMotionType::STATIC, 1.0f,
                  false);
          set_static_body(l_Body);
        }

        set_initialized(true);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_rebuild
      }

      uint32_t ConvexHullCollider::create_instance(u32 &p_PageIndex,
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

      u32 ConvexHullCollider::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT(
            (l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
            "Could not increase capacity for ConvexHullCollider.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, ConvexHullCollider::Data::get_size(),
            ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool ConvexHullCollider::get_page_for_index(const u32 p_Index,
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
