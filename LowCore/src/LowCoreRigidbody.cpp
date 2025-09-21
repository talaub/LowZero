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
#include "LowCorePhysicsSystem.h"
#include "LowCoreTransform.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t Rigidbody::TYPE_ID = 31;
      uint32_t Rigidbody::ms_Capacity = 0u;
      uint32_t Rigidbody::ms_PageSize = 0u;
      Low::Util::SharedMutex Rigidbody::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          Rigidbody::ms_PagesLock(Rigidbody::ms_PagesMutex,
                                  std::defer_lock);
      Low::Util::List<Rigidbody> Rigidbody::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          Rigidbody::ms_Pages;

      Rigidbody::Rigidbody() : Low::Util::Handle(0ull)
      {
      }
      Rigidbody::Rigidbody(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Rigidbody::Rigidbody(Rigidbody &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

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
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        Rigidbody l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Rigidbody::TYPE_ID;

        l_PageLock.unlock();

        Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);

        ACCESSOR_TYPE_SOA(l_Handle, Rigidbody, fixed, bool) = false;
        ACCESSOR_TYPE_SOA(l_Handle, Rigidbody, gravity, bool) = false;
        ACCESSOR_TYPE_SOA(l_Handle, Rigidbody, mass, float) = 0.0f;
        ACCESSOR_TYPE_SOA(l_Handle, Rigidbody, initialized, bool) =
            false;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Rigidbody, rigid_dynamic,
                                   PhysicsRigidDynamic))
            PhysicsRigidDynamic();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Rigidbody, physics_shape,
                                   PhysicsShape)) PhysicsShape();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Rigidbody, shape,
                                   Math::Shape)) Math::Shape();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Rigidbody, entity,
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

        {
          Math::Shape l_Shape;
          l_Shape.type = Math::ShapeType::BOX;
          l_Shape.box.position = Math::Vector3(0.0f);
          l_Shape.box.rotation = Math::Quaternion();
          l_Shape.box.halfExtents = Math::Vector3(0.5f);

          l_Handle.set_shape(l_Shape);
        }

        l_Handle.set_gravity(false);
        l_Handle.set_mass(1.0f);
        l_Handle.set_fixed(true);
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Rigidbody::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<Rigidbody> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

          System::Physics::remove_rigid_dynamic(get_rigid_dynamic());
          get_rigid_dynamic().destroy();
          get_physics_shape().destroy();
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

        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
        l_Page->slots[l_SlotIndex].m_Occupied = false;
        l_Page->slots[l_SlotIndex].m_Generation++;

        ms_PagesLock.lock();
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end();) {
          if (it->get_id() == get_id()) {
            it = ms_LivingInstances.erase(it);
          } else {
            it++;
          }
        }
        ms_PagesLock.unlock();
      }

      void Rigidbody::initialize()
      {
        LOCK_PAGES_WRITE(l_PagesLock);
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
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Rigidbody);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Rigidbody::is_alive;
        l_TypeInfo.destroy = &Rigidbody::destroy;
        l_TypeInfo.serialize = &Rigidbody::serialize;
        l_TypeInfo.deserialize = &Rigidbody::deserialize;
        l_TypeInfo.find_by_index = &Rigidbody::_find_by_index;
        l_TypeInfo.notify = &Rigidbody::_notify;
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
          // Property: fixed
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(fixed);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(Rigidbody::Data, fixed);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Rigidbody l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
            l_Handle.is_fixed();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Rigidbody,
                                              fixed, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.set_fixed(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Rigidbody l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
            *((bool *)p_Data) = l_Handle.is_fixed();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: fixed
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
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
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
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
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
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
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
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
            *((float *)p_Data) = l_Handle.get_mass();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: mass
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
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
            l_Handle.is_initialized();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Rigidbody,
                                              initialized, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Rigidbody l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
            *((bool *)p_Data) = l_Handle.is_initialized();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: initialized
        }
        {
          // Property: rigid_dynamic
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(rigid_dynamic);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Rigidbody::Data, rigid_dynamic);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Rigidbody l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
            l_Handle.get_rigid_dynamic();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Rigidbody,
                                              rigid_dynamic,
                                              PhysicsRigidDynamic);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Rigidbody l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
            *((PhysicsRigidDynamic *)p_Data) =
                l_Handle.get_rigid_dynamic();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: rigid_dynamic
        }
        {
          // Property: physics_shape
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(physics_shape);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Rigidbody::Data, physics_shape);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
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
          // End property: physics_shape
        }
        {
          // Property: shape
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(shape);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(Rigidbody::Data, shape);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::SHAPE;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Rigidbody l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
            l_Handle.get_shape();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Rigidbody,
                                              shape, Math::Shape);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Rigidbody l_Handle = p_Handle.get_id();
            l_Handle.set_shape(*(Math::Shape *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Rigidbody l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
            *((Math::Shape *)p_Data) = l_Handle.get_shape();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: shape
        }
        {
          // Property: entity
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Rigidbody::Data, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::TYPE_ID;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Rigidbody l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
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
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
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
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Rigidbody, unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Rigidbody l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Rigidbody> l_HandleLock(l_Handle);
            *((Low::Util::UniqueId *)p_Data) =
                l_Handle.get_unique_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: unique_id
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void Rigidbody::cleanup()
      {
        Low::Util::List<Rigidbody> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        ms_PagesLock.lock();
        for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
          Low::Util::Instances::Page *i_Page = *it;
          free(i_Page->buffer);
          free(i_Page->slots);
          free(i_Page->lockWords);
          delete i_Page;
          it = ms_Pages.erase(it);
        }

        ms_Capacity = 0;

        ms_PagesLock.unlock();
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
        l_Handle.m_Data.m_Type = Rigidbody::TYPE_ID;

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
          l_Handle.m_Data.m_Generation = 0;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
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
        l_Handle.m_Data.m_Type = Rigidbody::TYPE_ID;

        return l_Handle;
      }

      bool Rigidbody::is_alive() const
      {
        if (m_Data.m_Type != Rigidbody::TYPE_ID) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
        return m_Data.m_Type == Rigidbody::TYPE_ID &&
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
        l_Handle.set_fixed(is_fixed());
        l_Handle.set_gravity(is_gravity());
        l_Handle.set_mass(get_mass());
        l_Handle.set_initialized(is_initialized());
        l_Handle.set_shape(get_shape());

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

      void Rigidbody::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["fixed"] = is_fixed();
        p_Node["gravity"] = is_gravity();
        p_Node["mass"] = get_mass();
        Low::Util::Serialization::serialize(p_Node["shape"],
                                            get_shape());
        p_Node["_unique_id"] =
            Low::Util::hash_to_string(get_unique_id()).c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Rigidbody::serialize(Low::Util::Handle p_Handle,
                                Low::Util::Yaml::Node &p_Node)
      {
        Rigidbody l_Rigidbody = p_Handle.get_id();
        l_Rigidbody.serialize(p_Node);
      }

      Low::Util::Handle
      Rigidbody::deserialize(Low::Util::Yaml::Node &p_Node,
                             Low::Util::Handle p_Creator)
      {
        Low::Util::UniqueId l_HandleUniqueId = 0ull;
        if (p_Node["unique_id"]) {
          l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
        } else if (p_Node["_unique_id"]) {
          l_HandleUniqueId = Low::Util::string_to_hash(
              LOW_YAML_AS_STRING(p_Node["_unique_id"]));
        }

        Rigidbody l_Handle =
            Rigidbody::make(p_Creator.get_id(), l_HandleUniqueId);

        if (p_Node["fixed"]) {
          l_Handle.set_fixed(p_Node["fixed"].as<bool>());
        }
        if (p_Node["gravity"]) {
          l_Handle.set_gravity(p_Node["gravity"].as<bool>());
        }
        if (p_Node["mass"]) {
          l_Handle.set_mass(p_Node["mass"].as<float>());
        }
        if (p_Node["shape"]) {
          l_Handle.set_shape(
              Low::Util::Serialization::deserialize_shape(
                  p_Node["shape"]));
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

      bool Rigidbody::is_fixed() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_fixed

        // LOW_CODEGEN::END::CUSTOM:GETTER_fixed

        return TYPE_SOA(Rigidbody, fixed, bool);
      }
      void Rigidbody::toggle_fixed()
      {
        set_fixed(!is_fixed());
      }

      void Rigidbody::set_fixed(bool p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_fixed

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_fixed

        // Set new value
        TYPE_SOA(Rigidbody, fixed, bool) = p_Value;
        {
          Low::Core::Entity l_Entity = get_entity();
          if (l_Entity.has_component(
                  Low::Core::Component::PrefabInstance::TYPE_ID)) {
            Low::Core::Component::PrefabInstance l_Instance =
                l_Entity.get_component(
                    Low::Core::Component::PrefabInstance::TYPE_ID);
            Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
            if (l_Prefab.is_alive()) {
              l_Instance.override(
                  TYPE_ID, N(fixed),
                  !l_Prefab.compare_property(*this, N(fixed)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_fixed

        get_rigid_dynamic().set_fixed(p_Value);
        // LOW_CODEGEN::END::CUSTOM:SETTER_fixed

        broadcast_observable(N(fixed));
      }

      bool Rigidbody::is_gravity() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

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
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gravity

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_gravity

        // Set new value
        TYPE_SOA(Rigidbody, gravity, bool) = p_Value;
        {
          Low::Core::Entity l_Entity = get_entity();
          if (l_Entity.has_component(
                  Low::Core::Component::PrefabInstance::TYPE_ID)) {
            Low::Core::Component::PrefabInstance l_Instance =
                l_Entity.get_component(
                    Low::Core::Component::PrefabInstance::TYPE_ID);
            Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
            if (l_Prefab.is_alive()) {
              l_Instance.override(
                  TYPE_ID, N(gravity),
                  !l_Prefab.compare_property(*this, N(gravity)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gravity

        get_rigid_dynamic().set_gravity(p_Value);
        // LOW_CODEGEN::END::CUSTOM:SETTER_gravity

        broadcast_observable(N(gravity));
      }

      float Rigidbody::get_mass() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mass

        // LOW_CODEGEN::END::CUSTOM:GETTER_mass

        return TYPE_SOA(Rigidbody, mass, float);
      }
      void Rigidbody::set_mass(float p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mass

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_mass

        // Set new value
        TYPE_SOA(Rigidbody, mass, float) = p_Value;
        {
          Low::Core::Entity l_Entity = get_entity();
          if (l_Entity.has_component(
                  Low::Core::Component::PrefabInstance::TYPE_ID)) {
            Low::Core::Component::PrefabInstance l_Instance =
                l_Entity.get_component(
                    Low::Core::Component::PrefabInstance::TYPE_ID);
            Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
            if (l_Prefab.is_alive()) {
              l_Instance.override(
                  TYPE_ID, N(mass),
                  !l_Prefab.compare_property(*this, N(mass)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mass

        get_rigid_dynamic().set_mass(p_Value);
        // LOW_CODEGEN::END::CUSTOM:SETTER_mass

        broadcast_observable(N(mass));
      }

      bool Rigidbody::is_initialized() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

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
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_initialized

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_initialized

        // Set new value
        TYPE_SOA(Rigidbody, initialized, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_initialized

        // LOW_CODEGEN::END::CUSTOM:SETTER_initialized

        broadcast_observable(N(initialized));
      }

      PhysicsRigidDynamic &Rigidbody::get_rigid_dynamic() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_rigid_dynamic

        // LOW_CODEGEN::END::CUSTOM:GETTER_rigid_dynamic

        return TYPE_SOA(Rigidbody, rigid_dynamic,
                        PhysicsRigidDynamic);
      }

      PhysicsShape &Rigidbody::get_physics_shape() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_physics_shape

        // LOW_CODEGEN::END::CUSTOM:GETTER_physics_shape

        return TYPE_SOA(Rigidbody, physics_shape, PhysicsShape);
      }

      Math::Shape &Rigidbody::get_shape() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_shape

        // LOW_CODEGEN::END::CUSTOM:GETTER_shape

        return TYPE_SOA(Rigidbody, shape, Math::Shape);
      }
      void Rigidbody::set_shape(Math::Shape &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_shape

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_shape

        // Set new value
        TYPE_SOA(Rigidbody, shape, Math::Shape) = p_Value;
        {
          Low::Core::Entity l_Entity = get_entity();
          if (l_Entity.has_component(
                  Low::Core::Component::PrefabInstance::TYPE_ID)) {
            Low::Core::Component::PrefabInstance l_Instance =
                l_Entity.get_component(
                    Low::Core::Component::PrefabInstance::TYPE_ID);
            Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
            if (l_Prefab.is_alive()) {
              l_Instance.override(
                  TYPE_ID, N(shape),
                  !l_Prefab.compare_property(*this, N(shape)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_shape

        if (!is_initialized()) {
          Component::Transform l_Transform =
              get_entity().get_transform();

          PhysicsShape::create(get_physics_shape(), p_Value);

          void *l_UserData = (void *)TYPE_SOA_PTR(
              Rigidbody, unique_id, Util::UniqueId);

          PhysicsRigidDynamic::create(
              get_rigid_dynamic(), get_physics_shape(),
              l_Transform.get_world_position(),
              l_Transform.get_world_rotation(), l_UserData);

          System::Physics::register_rigid_dynamic(
              get_rigid_dynamic());

          set_initialized(true);
        } else {
          LOW_LOG_DEBUG << "Updating physx shape" << LOW_LOG_END;
          get_physics_shape().update(p_Value);
          get_rigid_dynamic().update_shape(get_physics_shape());
        }

        // LOW_CODEGEN::END::CUSTOM:SETTER_shape

        broadcast_observable(N(shape));
      }

      Low::Core::Entity Rigidbody::get_entity() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity

        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        return TYPE_SOA(Rigidbody, entity, Low::Core::Entity);
      }
      void Rigidbody::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

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
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(Rigidbody, unique_id, Low::Util::UniqueId);
      }
      void Rigidbody::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Rigidbody> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(Rigidbody, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

        broadcast_observable(N(unique_id));
      }

      uint32_t Rigidbody::create_instance(
          u32 &p_PageIndex, u32 &p_SlotIndex,
          Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock)
      {
        LOCK_PAGES_WRITE(l_PagesLock);
        u32 l_Index = 0;
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        bool l_FoundIndex = false;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;

        for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
             ++l_PageIndex) {
          Low::Util::UniqueLock<Low::Util::Mutex> i_PageLock(
              ms_Pages[l_PageIndex]->mutex);
          for (l_SlotIndex = 0;
               l_SlotIndex < ms_Pages[l_PageIndex]->size;
               ++l_SlotIndex) {
            if (!ms_Pages[l_PageIndex]
                     ->slots[l_SlotIndex]
                     .m_Occupied) {
              l_FoundIndex = true;
              l_PageLock = std::move(i_PageLock);
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
          Low::Util::UniqueLock<Low::Util::Mutex> l_NewLock(
              ms_Pages[l_PageIndex]->mutex);
          l_PageLock = std::move(l_NewLock);
        }
        ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
        p_PageIndex = l_PageIndex;
        p_SlotIndex = l_SlotIndex;
        p_PageLock = std::move(l_PageLock);
        LOCK_UNLOCK(l_PagesLock);
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
