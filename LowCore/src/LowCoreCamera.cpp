#include "LowCoreCamera.h"

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

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t Camera::TYPE_ID = 44;
      uint32_t Camera::ms_Capacity = 0u;
      uint32_t Camera::ms_PageSize = 0u;
      Low::Util::SharedMutex Camera::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          Camera::ms_PagesLock(Camera::ms_PagesMutex,
                               std::defer_lock);
      Low::Util::List<Camera> Camera::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *> Camera::ms_Pages;

      Camera::Camera() : Low::Util::Handle(0ull)
      {
      }
      Camera::Camera(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Camera::Camera(Camera &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle Camera::_make(Low::Util::Handle p_Entity)
      {
        Low::Core::Entity l_Entity = p_Entity.get_id();
        LOW_ASSERT(l_Entity.is_alive(),
                   "Cannot create component for dead entity");
        return make(l_Entity).get_id();
      }

      Camera Camera::make(Low::Core::Entity p_Entity)
      {
        return make(p_Entity, 0ull);
      }

      Camera Camera::make(Low::Core::Entity p_Entity,
                          Low::Util::UniqueId p_UniqueId)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        Camera l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Camera::TYPE_ID;

        l_PageLock.unlock();

        Low::Util::HandleLock<Camera> l_HandleLock(l_Handle);

        ACCESSOR_TYPE_SOA(l_Handle, Camera, active, bool) = false;
        ACCESSOR_TYPE_SOA(l_Handle, Camera, fov, float) = 0.0f;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Camera, entity,
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

        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Camera::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<Camera> l_Lock(get_id());
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

      void Camera::initialize()
      {
        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Camera));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Camera::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Camera);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Camera::is_alive;
        l_TypeInfo.destroy = &Camera::destroy;
        l_TypeInfo.serialize = &Camera::serialize;
        l_TypeInfo.deserialize = &Camera::deserialize;
        l_TypeInfo.find_by_index = &Camera::_find_by_index;
        l_TypeInfo.notify = &Camera::_notify;
        l_TypeInfo.make_default = nullptr;
        l_TypeInfo.make_component = &Camera::_make;
        l_TypeInfo.duplicate_default = nullptr;
        l_TypeInfo.duplicate_component = &Camera::_duplicate;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Camera::living_instances);
        l_TypeInfo.get_living_count = &Camera::living_count;
        l_TypeInfo.component = true;
        l_TypeInfo.uiComponent = false;
        {
          // Property: active
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(active);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Camera::Data, active);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Camera l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Camera> l_HandleLock(l_Handle);
            l_Handle.is_active();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Camera,
                                              active, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Camera l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Camera> l_HandleLock(l_Handle);
            *((bool *)p_Data) = l_Handle.is_active();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: active
        }
        {
          // Property: fov
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(fov);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(Camera::Data, fov);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Camera l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Camera> l_HandleLock(l_Handle);
            l_Handle.get_fov();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Camera, fov,
                                              float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Camera l_Handle = p_Handle.get_id();
            l_Handle.set_fov(*(float *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Camera l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Camera> l_HandleLock(l_Handle);
            *((float *)p_Data) = l_Handle.get_fov();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: fov
        }
        {
          // Property: entity
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Camera::Data, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::TYPE_ID;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Camera l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Camera> l_HandleLock(l_Handle);
            l_Handle.get_entity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Camera, entity, Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Camera l_Handle = p_Handle.get_id();
            l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Camera l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Camera> l_HandleLock(l_Handle);
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
              offsetof(Camera::Data, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Camera l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Camera> l_HandleLock(l_Handle);
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Camera, unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Camera l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Camera> l_HandleLock(l_Handle);
            *((Low::Util::UniqueId *)p_Data) =
                l_Handle.get_unique_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: unique_id
        }
        {
          // Function: activate
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(activate);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: activate
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void Camera::cleanup()
      {
        Low::Util::List<Camera> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle Camera::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Camera Camera::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Camera l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Camera::TYPE_ID;

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

      Camera Camera::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Camera l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Camera::TYPE_ID;

        return l_Handle;
      }

      bool Camera::is_alive() const
      {
        if (m_Data.m_Type != Camera::TYPE_ID) {
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
        return m_Data.m_Type == Camera::TYPE_ID &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Camera::get_capacity()
      {
        return ms_Capacity;
      }

      Camera Camera::duplicate(Low::Core::Entity p_Entity) const
      {
        _LOW_ASSERT(is_alive());

        Camera l_Handle = make(p_Entity);
        l_Handle.set_active(is_active());
        l_Handle.set_fov(get_fov());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      Camera Camera::duplicate(Camera p_Handle,
                               Low::Core::Entity p_Entity)
      {
        return p_Handle.duplicate(p_Entity);
      }

      Low::Util::Handle Camera::_duplicate(Low::Util::Handle p_Handle,
                                           Low::Util::Handle p_Entity)
      {
        Camera l_Camera = p_Handle.get_id();
        Low::Core::Entity l_Entity = p_Entity.get_id();
        return l_Camera.duplicate(l_Entity);
      }

      void Camera::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["active"] = is_active();
        p_Node["fov"] = get_fov();
        p_Node["_unique_id"] =
            Low::Util::hash_to_string(get_unique_id()).c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Camera::serialize(Low::Util::Handle p_Handle,
                             Low::Util::Yaml::Node &p_Node)
      {
        Camera l_Camera = p_Handle.get_id();
        l_Camera.serialize(p_Node);
      }

      Low::Util::Handle
      Camera::deserialize(Low::Util::Yaml::Node &p_Node,
                          Low::Util::Handle p_Creator)
      {
        Low::Util::UniqueId l_HandleUniqueId = 0ull;
        if (p_Node["unique_id"]) {
          l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
        } else if (p_Node["_unique_id"]) {
          l_HandleUniqueId = Low::Util::string_to_hash(
              LOW_YAML_AS_STRING(p_Node["_unique_id"]));
        }

        Camera l_Handle =
            Camera::make(p_Creator.get_id(), l_HandleUniqueId);

        if (p_Node["active"]) {
          l_Handle.set_active(p_Node["active"].as<bool>());
        }
        if (p_Node["fov"]) {
          l_Handle.set_fov(p_Node["fov"].as<float>());
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
      Camera::broadcast_observable(Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 Camera::observe(Low::Util::Name p_Observable,
                          Low::Util::Function<void(Low::Util::Handle,
                                                   Low::Util::Name)>
                              p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Camera::observe(Low::Util::Name p_Observable,
                          Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Camera::notify(Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Camera::_notify(Low::Util::Handle p_Observer,
                           Low::Util::Handle p_Observed,
                           Low::Util::Name p_Observable)
      {
        Camera l_Camera = p_Observer.get_id();
        l_Camera.notify(p_Observed, p_Observable);
      }

      bool Camera::is_active() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Camera> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_active

        // LOW_CODEGEN::END::CUSTOM:GETTER_active

        return TYPE_SOA(Camera, active, bool);
      }
      void Camera::toggle_active()
      {
        set_active(!is_active());
      }

      void Camera::set_active(bool p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Camera> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_active

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_active

        // Set new value
        TYPE_SOA(Camera, active, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_active

        // LOW_CODEGEN::END::CUSTOM:SETTER_active

        broadcast_observable(N(active));
      }

      float Camera::get_fov() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Camera> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_fov

        // LOW_CODEGEN::END::CUSTOM:GETTER_fov

        return TYPE_SOA(Camera, fov, float);
      }
      void Camera::set_fov(float p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Camera> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_fov

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_fov

        // Set new value
        TYPE_SOA(Camera, fov, float) = p_Value;
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
                  TYPE_ID, N(fov),
                  !l_Prefab.compare_property(*this, N(fov)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_fov

        // LOW_CODEGEN::END::CUSTOM:SETTER_fov

        broadcast_observable(N(fov));
      }

      Low::Core::Entity Camera::get_entity() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Camera> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity

        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        return TYPE_SOA(Camera, entity, Low::Core::Entity);
      }
      void Camera::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Camera> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

        // Set new value
        TYPE_SOA(Camera, entity, Low::Core::Entity) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity

        // LOW_CODEGEN::END::CUSTOM:SETTER_entity

        broadcast_observable(N(entity));
      }

      Low::Util::UniqueId Camera::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Camera> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(Camera, unique_id, Low::Util::UniqueId);
      }
      void Camera::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Camera> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(Camera, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

        broadcast_observable(N(unique_id));
      }

      void Camera::activate()
      {
        Low::Util::HandleLock<Camera> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_activate

        for (Camera i_Camera : ms_LivingInstances) {
          i_Camera.set_active(false);
        }
        set_active(true);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_activate
      }

      uint32_t Camera::create_instance(
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

      u32 Camera::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Camera.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Camera::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Camera::get_page_for_index(const u32 p_Index,
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
