#include "LowCoreRigidbody.h"

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
#include "LowCoreBoxCollider.h"
#include "LowCoreSphereCollider.h"
#include "LowCoreTransform.h"
#include "LowCoreRegion.h"
#include "LowCoreScene.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      Low::Util::Set<Low::Core::Component::Rigidbody>
          Low::Core::Component::Rigidbody::ms_Dirty;

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

      u16 Rigidbody::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          Rigidbody::IDENTIFIER(LOW_NAME(1181529166),
                                LOW_NAME(2193485588));
      uint32_t Rigidbody::ms_Capacity = 0u;
      uint32_t Rigidbody::ms_PageSize = 0u;
      Low::Util::List<Rigidbody> Rigidbody::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          Rigidbody::ms_Pages;

      Low::Util::Handle Rigidbody::_make(Low::Util::Handle p_Entity)
      {
        Low::Core::Entity l_Entity = p_Entity.get_id();
        LOW_ASSERT(l_Entity.is_alive(),
                   "Cannot create component for dead entity");
        return make(l_Entity).get_id();
      }

      Rigidbody Rigidbody::make(Low::Core::Entity p_Entity)
      {
        return make(p_Entity, 0ull);
      }

      Rigidbody Rigidbody::make(Low::Core::Entity p_Entity,
                                Low::Util::UniqueId p_UniqueId)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        Rigidbody l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Rigidbody::ms_TypeId;

        new (
            ACCESSOR_TYPE_SOA_PTR(l_Handle, Rigidbody, motion_type,
                                  Low::Core::Physics::BodyMotionType))
            Low::Core::Physics::BodyMotionType();
        ACCESSOR_TYPE_SOA(l_Handle, Rigidbody, gravity, bool) = false;
        ACCESSOR_TYPE_SOA(l_Handle, Rigidbody, mass, float) = 0.0f;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Rigidbody, body,
                                   Low::Core::Physics::Body))
            Low::Core::Physics::Body();
        ACCESSOR_TYPE_SOA(l_Handle, Rigidbody, initialized, bool) =
            false;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Rigidbody, entity,
                                   Low::Core::Entity))
            Low::Core::Entity();
        ACCESSOR_TYPE_SOA(l_Handle, Rigidbody, dirty, bool) = false;

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
        ACCESSOR_TYPE_SOA(l_Handle, Rigidbody, motion_type,
                          Low::Core::Physics::BodyMotionType) =
            Low::Core::Physics::BodyMotionType::DYNAMIC;
        ACCESSOR_TYPE_SOA(l_Handle, Rigidbody, gravity, bool) = true;
        ACCESSOR_TYPE_SOA(l_Handle, Rigidbody, mass, float) = 1.0f;
        l_Handle.mark_dirty();
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Rigidbody::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          if (get_body().is_alive()) {
            get_body().destroy();
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

      void Rigidbody::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowCore), N(Rigidbody));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Rigidbody));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Rigidbody::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Rigidbody);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Rigidbody::is_alive;
        l_TypeInfo.destroy = &Rigidbody::destroy;
        l_TypeInfo.serialize = &Rigidbody::serialize;
        l_TypeInfo.deserialize = &Rigidbody::deserialize;
        l_TypeInfo.find_by_index = &Rigidbody::_find_by_index;
        l_TypeInfo.notify = &Rigidbody::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.make_default = nullptr;
        l_TypeInfo.make_component = &Rigidbody::_make;
        l_TypeInfo.duplicate_default = nullptr;
        l_TypeInfo.duplicate_component = &Rigidbody::_duplicate;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Rigidbody::living_instances);
        l_TypeInfo.get_living_count = &Rigidbody::living_count;
        l_TypeInfo.component = true;
        l_TypeInfo.uiComponent = false;
        {
          // Property: motion_type
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(motion_type);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(Rigidbody::Data, motion_type);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
          l_PropertyInfo.handleType = Low::Core::Physics::
              BodyMotionTypeEnumHelper::get_enum_id();
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.get_motion_type();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Rigidbody, motion_type,
                Low::Core::Physics::BodyMotionType);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.set_motion_type(
                *(Low::Core::Physics::BodyMotionType *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Rigidbody l_Handle = p_Handle.get_id();
            *((Low::Core::Physics::BodyMotionType *)p_Data) =
                l_Handle.get_motion_type();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: motion_type
        }
        {
          // Property: gravity
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(gravity);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(Rigidbody::Data, gravity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.is_gravity();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Rigidbody,
                                              gravity, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.set_gravity(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Rigidbody l_Handle = p_Handle.get_id();
            *((bool *)p_Data) = l_Handle.is_gravity();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: gravity
        }
        {
          // Property: mass
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(mass);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(Rigidbody::Data, mass);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.get_mass();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Rigidbody,
                                              mass, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.set_mass(*(float *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Rigidbody l_Handle = p_Handle.get_id();
            *((float *)p_Data) = l_Handle.get_mass();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: mass
        }
        {
          // Property: body
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(body);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Rigidbody::Data, body);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Core::Physics::Body::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.get_body();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Rigidbody, body, Low::Core::Physics::Body);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Rigidbody l_Handle = p_Handle.get_id();
            *((Low::Core::Physics::Body *)p_Data) =
                l_Handle.get_body();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: body
        }
        {
          // Property: initialized
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(initialized);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Rigidbody::Data, initialized);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.is_initialized();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Rigidbody,
                                              initialized, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Rigidbody l_Handle = p_Handle.get_id();
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
              offsetof(Rigidbody::Data, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.get_entity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Rigidbody, entity, Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Rigidbody l_Handle = p_Handle.get_id();
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
              offsetof(Rigidbody::Data, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Rigidbody, unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Rigidbody l_Handle = p_Handle.get_id();
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
              offsetof(Rigidbody::Data, dirty);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.is_dirty();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Rigidbody,
                                              dirty, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.set_dirty(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Rigidbody l_Handle = p_Handle.get_id();
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

      void Rigidbody::cleanup()
      {
        Low::Util::List<Rigidbody> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle Rigidbody::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Rigidbody Rigidbody::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Rigidbody l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Rigidbody::ms_TypeId;

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

      Rigidbody Rigidbody::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Rigidbody l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Rigidbody::ms_TypeId;

        return l_Handle;
      }

      bool Rigidbody::is_alive() const
      {
        if (m_Data.m_Type != Rigidbody::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == Rigidbody::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Rigidbody::get_capacity()
      {
        return ms_Capacity;
      }

      Rigidbody Rigidbody::duplicate(Low::Core::Entity p_Entity) const
      {
        _LOW_ASSERT(is_alive());

        Rigidbody l_Handle = make(p_Entity);
        l_Handle.set_motion_type(get_motion_type());
        l_Handle.set_gravity(is_gravity());
        l_Handle.set_mass(get_mass());
        if (get_body().is_alive()) {
          l_Handle.set_body(get_body());
        }
        l_Handle.set_initialized(is_initialized());
        l_Handle.set_dirty(is_dirty());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      Rigidbody Rigidbody::duplicate(Rigidbody p_Handle,
                                     Low::Core::Entity p_Entity)
      {
        return p_Handle.duplicate(p_Entity);
      }

      Low::Util::Handle
      Rigidbody::_duplicate(Low::Util::Handle p_Handle,
                            Low::Util::Handle p_Entity)
      {
        Rigidbody l_Rigidbody = p_Handle.get_id();
        Low::Core::Entity l_Entity = p_Entity.get_id();
        return l_Rigidbody.duplicate(l_Entity);
      }

      void Rigidbody::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        Low::Util::Serial::serialize_enum(
            p_Node["motion_type"],
            Low::Core::Physics::BodyMotionTypeEnumHelper::
                get_enum_id(),
            static_cast<uint8_t>(get_motion_type()));
        p_Node["gravity"] = is_gravity();
        p_Node["mass"] = get_mass();
        p_Node["_unique_id"] = Low::Util::U64Id{get_unique_id()};

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Rigidbody::serialize(Low::Util::Handle p_Handle,
                                Low::Util::Serial::Node &p_Node)
      {
        Rigidbody l_Rigidbody = p_Handle.get_id();
        l_Rigidbody.serialize(p_Node);
      }

      Low::Util::Handle
      Rigidbody::deserialize(Low::Util::Serial::Node &p_Node,
                             Low::Util::Handle p_Creator)
      {
        Low::Util::UniqueId l_HandleUniqueId = 0ull;
        if (p_Node["unique_id"]) {
          l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
        } else if (p_Node["_unique_id"]) {
          l_HandleUniqueId = Low::Util::string_to_hash(
              p_Node["_unique_id"].as<Low::Util::String>());
        }

        Rigidbody l_Handle =
            Rigidbody::make(p_Creator.get_id(), l_HandleUniqueId);

        if (p_Node["motion_type"]) {
          l_Handle.set_motion_type(
              static_cast<Low::Core::Physics::BodyMotionType>(
                  Low::Util::Serial::deserialize_enum(
                      p_Node["motion_type"])));
        }
        if (p_Node["gravity"]) {
          l_Handle.set_gravity(p_Node["gravity"].as<bool>());
        }
        if (p_Node["mass"]) {
          l_Handle.set_mass(p_Node["mass"].as<float>());
        }
        if (p_Node["unique_id"]) {
          l_Handle.set_unique_id(
              p_Node["unique_id"].as<Low::Util::UniqueId>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void Rigidbody::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64
      Rigidbody::observe(Low::Util::Name p_Observable,
                         Low::Util::Function<void(Low::Util::Handle,
                                                  Low::Util::Name)>
                             p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Rigidbody::observe(Low::Util::Name p_Observable,
                             Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Rigidbody::notify(Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY

        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Rigidbody::_notify(Low::Util::Handle p_Observer,
                              Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
      {
        Rigidbody l_Rigidbody = p_Observer.get_id();
        l_Rigidbody.notify(p_Observed, p_Observable);
      }

      Low::Core::Physics::BodyMotionType
      Rigidbody::get_motion_type() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_motion_type
        // LOW_CODEGEN::END::CUSTOM:GETTER_motion_type

        return TYPE_SOA(Rigidbody, motion_type,
                        Low::Core::Physics::BodyMotionType);
      }
      void Rigidbody::set_motion_type(
          Low::Core::Physics::BodyMotionType p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_motion_type
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_motion_type

        if (get_motion_type() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(Rigidbody, motion_type,
                   Low::Core::Physics::BodyMotionType) = p_Value;
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
                l_Instance.override(ms_TypeId, N(motion_type),
                                    !l_Prefab.compare_property(
                                        *this, N(motion_type)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_motion_type
          rebuild();
          // LOW_CODEGEN::END::CUSTOM:SETTER_motion_type

          broadcast_observable(N(motion_type));
        }
      }

      bool Rigidbody::is_gravity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gravity

        // LOW_CODEGEN::END::CUSTOM:GETTER_gravity

        return TYPE_SOA(Rigidbody, gravity, bool);
      }
      void Rigidbody::toggle_gravity()
      {
        set_gravity(!is_gravity());
      }

      void Rigidbody::set_gravity(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gravity

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_gravity

        if (is_gravity() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(Rigidbody, gravity, bool) = p_Value;
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
                    ms_TypeId, N(gravity),
                    !l_Prefab.compare_property(*this, N(gravity)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gravity
          // LOW_CODEGEN::END::CUSTOM:SETTER_gravity

          broadcast_observable(N(gravity));
        }
      }

      float Rigidbody::get_mass() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mass

        // LOW_CODEGEN::END::CUSTOM:GETTER_mass

        return TYPE_SOA(Rigidbody, mass, float);
      }
      void Rigidbody::set_mass(float p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mass

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_mass

        if (get_mass() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(Rigidbody, mass, float) = p_Value;
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
                    ms_TypeId, N(mass),
                    !l_Prefab.compare_property(*this, N(mass)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mass
          // LOW_CODEGEN::END::CUSTOM:SETTER_mass

          broadcast_observable(N(mass));
        }
      }

      Low::Core::Physics::Body Rigidbody::get_body() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_body
        // LOW_CODEGEN::END::CUSTOM:GETTER_body

        return TYPE_SOA(Rigidbody, body, Low::Core::Physics::Body);
      }
      void Rigidbody::set_body(Low::Core::Physics::Body p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_body
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_body

        // Set new value
        TYPE_SOA(Rigidbody, body, Low::Core::Physics::Body) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_body
        // LOW_CODEGEN::END::CUSTOM:SETTER_body

        broadcast_observable(N(body));
      }

      bool Rigidbody::is_initialized() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_initialized

        // LOW_CODEGEN::END::CUSTOM:GETTER_initialized

        return TYPE_SOA(Rigidbody, initialized, bool);
      }
      void Rigidbody::toggle_initialized()
      {
        set_initialized(!is_initialized());
      }

      void Rigidbody::set_initialized(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_initialized

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_initialized

        // Set new value
        TYPE_SOA(Rigidbody, initialized, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_initialized

        // LOW_CODEGEN::END::CUSTOM:SETTER_initialized

        broadcast_observable(N(initialized));
      }

      Low::Core::Entity Rigidbody::get_entity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity

        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        return TYPE_SOA(Rigidbody, entity, Low::Core::Entity);
      }
      void Rigidbody::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

        // Set new value
        TYPE_SOA(Rigidbody, entity, Low::Core::Entity) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity

        // LOW_CODEGEN::END::CUSTOM:SETTER_entity

        broadcast_observable(N(entity));
      }

      Low::Util::UniqueId Rigidbody::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(Rigidbody, unique_id, Low::Util::UniqueId);
      }
      void Rigidbody::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(Rigidbody, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

        broadcast_observable(N(unique_id));
      }

      bool Rigidbody::is_dirty() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

        return TYPE_SOA(Rigidbody, dirty, bool);
      }
      void Rigidbody::toggle_dirty()
      {
        set_dirty(!is_dirty());
      }

      void Rigidbody::set_dirty(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

        // Set new value
        TYPE_SOA(Rigidbody, dirty, bool) = p_Value;

        if (p_Value) {
          mark_dirty();
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:SETTER_dirty

        broadcast_observable(N(dirty));
      }

      void Rigidbody::mark_dirty()
      {
        if (!is_dirty()) {
          TYPE_SOA(Rigidbody, dirty, bool) = true;
          // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
          ms_Dirty.insert(get_id());
          // LOW_CODEGEN::END::CUSTOM:MARK_dirty
        }
      }

      void Rigidbody::rebuild()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_rebuild
        _LOW_ASSERT(is_alive());

        if (get_body().is_alive()) {
          get_body().destroy();
          set_body(Low::Core::Physics::Body());
        }

        Low::Core::Entity l_Entity = get_entity();
        _LOW_ASSERT(l_Entity.is_alive());

        Low::Core::Physics::Shape l_Shape;
        Low::Math::Vector3 l_Center(0.0f);

        if (l_Entity.has_component(BoxCollider::type_id())) {
          BoxCollider l_Collider =
              l_Entity.get_component(BoxCollider::type_id());
          if (!l_Collider.get_shape().is_alive()) {
            set_initialized(false);
            return;
          }
          if (l_Collider.get_static_body().is_alive()) {
            l_Collider.get_static_body().destroy();
          }
          l_Shape = l_Collider.get_shape();
          l_Center = l_Collider.get_center();
        } else if (l_Entity.has_component(
                       SphereCollider::type_id())) {
          SphereCollider l_Collider =
              l_Entity.get_component(SphereCollider::type_id());
          if (!l_Collider.get_shape().is_alive()) {
            set_initialized(false);
            return;
          }
          if (l_Collider.get_static_body().is_alive()) {
            l_Collider.get_static_body().destroy();
          }
          l_Shape = l_Collider.get_shape();
          l_Center = l_Collider.get_center();
        } else {
          set_initialized(false);
          return;
        }

        Low::Core::Component::Transform l_Transform =
            l_Entity.get_transform();
        _LOW_ASSERT(l_Transform.is_alive());

        Low::Core::Physics::World l_World =
            get_physics_world(l_Entity);

        Low::Math::Vector3 l_Position =
            l_Transform.get_world_position() +
            (l_Transform.get_world_rotation() * l_Center);

        Low::Core::Physics::Body l_Body =
            Low::Core::Physics::Body::make(
                l_World, l_Shape, l_Position,
                l_Transform.get_world_rotation(), get_motion_type(),
                get_mass(), is_gravity());
        l_Body.set_owner(get_id());
        set_body(l_Body);
        set_initialized(true);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_rebuild
      }

      uint32_t Rigidbody::create_instance(u32 &p_PageIndex,
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

      u32 Rigidbody::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Rigidbody.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Rigidbody::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Rigidbody::get_page_for_index(const u32 p_Index,
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
