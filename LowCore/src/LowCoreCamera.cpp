#include "LowCoreCamera.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

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
      uint8_t *Camera::ms_Buffer = 0;
      Low::Util::Instances::Slot *Camera::ms_Slots = 0;
      Low::Util::List<Camera> Camera::ms_LivingInstances =
          Low::Util::List<Camera>();

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
        uint32_t l_Index = create_instance();

        Camera l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Camera::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, Camera, active, bool) = false;
        ACCESSOR_TYPE_SOA(l_Handle, Camera, fov, float) = 0.0f;
        new (&ACCESSOR_TYPE_SOA(l_Handle, Camera, entity,
                                Low::Core::Entity))
            Low::Core::Entity();

        l_Handle.set_entity(p_Entity);
        p_Entity.add_component(l_Handle);

        ms_LivingInstances.push_back(l_Handle);

        l_Handle.set_unique_id(
            Low::Util::generate_unique_id(l_Handle.get_id()));
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Camera::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        // LOW_CODEGEN::END::CUSTOM:DESTROY

        Low::Util::remove_unique_id(get_unique_id());

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const Camera *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
      }

      void Camera::initialize()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Camera));

        initialize_buffer(&ms_Buffer, CameraData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_Camera);
        LOW_PROFILE_ALLOC(type_slots_Camera);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Camera);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Camera::is_alive;
        l_TypeInfo.destroy = &Camera::destroy;
        l_TypeInfo.serialize = &Camera::serialize;
        l_TypeInfo.deserialize = &Camera::deserialize;
        l_TypeInfo.find_by_index = &Camera::_find_by_index;
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
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(active);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(CameraData, active);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Camera l_Handle = p_Handle.get_id();
            l_Handle.is_active();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Camera,
                                              active, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(fov);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(CameraData, fov);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Camera l_Handle = p_Handle.get_id();
            l_Handle.get_fov();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Camera, fov,
                                              float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Camera l_Handle = p_Handle.get_id();
            l_Handle.set_fov(*(float *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(CameraData, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::TYPE_ID;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Camera l_Handle = p_Handle.get_id();
            l_Handle.get_entity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Camera, entity, Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Camera l_Handle = p_Handle.get_id();
            l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(CameraData, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Camera l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Camera, unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(activate);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void Camera::cleanup()
      {
        Low::Util::List<Camera> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_Camera);
        LOW_PROFILE_FREE(type_slots_Camera);
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
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = Camera::TYPE_ID;

        return l_Handle;
      }

      bool Camera::is_alive() const
      {
        return m_Data.m_Type == Camera::TYPE_ID &&
               check_alive(ms_Slots, Camera::get_capacity());
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
        p_Node["unique_id"] = get_unique_id();

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
        Camera l_Handle = Camera::make(p_Creator.get_id());

        if (p_Node["unique_id"]) {
          Low::Util::remove_unique_id(l_Handle.get_unique_id());
          l_Handle.set_unique_id(p_Node["unique_id"].as<uint64_t>());
          Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                        l_Handle.get_id());
        }

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

      bool Camera::is_active() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_active

        // LOW_CODEGEN::END::CUSTOM:GETTER_active

        return TYPE_SOA(Camera, active, bool);
      }
      void Camera::set_active(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_active

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_active

        // Set new value
        TYPE_SOA(Camera, active, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_active

        // LOW_CODEGEN::END::CUSTOM:SETTER_active
      }

      float Camera::get_fov() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_fov

        // LOW_CODEGEN::END::CUSTOM:GETTER_fov

        return TYPE_SOA(Camera, fov, float);
      }
      void Camera::set_fov(float p_Value)
      {
        _LOW_ASSERT(is_alive());

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
      }

      Low::Core::Entity Camera::get_entity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity

        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        return TYPE_SOA(Camera, entity, Low::Core::Entity);
      }
      void Camera::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

        // Set new value
        TYPE_SOA(Camera, entity, Low::Core::Entity) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity

        // LOW_CODEGEN::END::CUSTOM:SETTER_entity
      }

      Low::Util::UniqueId Camera::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(Camera, unique_id, Low::Util::UniqueId);
      }
      void Camera::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(Camera, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
      }

      void Camera::activate()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_activate

        for (Camera i_Camera : ms_LivingInstances) {
          i_Camera.set_active(false);
        }
        set_active(true);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_activate
      }

      uint32_t Camera::create_instance()
      {
        uint32_t l_Index = 0u;

        for (; l_Index < get_capacity(); ++l_Index) {
          if (!ms_Slots[l_Index].m_Occupied) {
            break;
          }
        }
        if (l_Index >= get_capacity()) {
          increase_budget();
        }
        ms_Slots[l_Index].m_Occupied = true;
        return l_Index;
      }

      void Camera::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease =
            std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0,
                   "Could not increase capacity");

        uint8_t *l_NewBuffer = (uint8_t *)malloc(
            (l_Capacity + l_CapacityIncrease) * sizeof(CameraData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(
              &l_NewBuffer[offsetof(CameraData, active) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(CameraData, active) * (l_Capacity)],
              l_Capacity * sizeof(bool));
        }
        {
          memcpy(&l_NewBuffer[offsetof(CameraData, fov) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(CameraData, fov) * (l_Capacity)],
                 l_Capacity * sizeof(float));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(CameraData, entity) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(CameraData, entity) * (l_Capacity)],
              l_Capacity * sizeof(Low::Core::Entity));
        }
        {
          memcpy(&l_NewBuffer[offsetof(CameraData, unique_id) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(CameraData, unique_id) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Util::UniqueId));
        }
        for (uint32_t i = l_Capacity;
             i < l_Capacity + l_CapacityIncrease; ++i) {
          l_NewSlots[i].m_Occupied = false;
          l_NewSlots[i].m_Generation = 0;
        }
        free(ms_Buffer);
        free(ms_Slots);
        ms_Buffer = l_NewBuffer;
        ms_Slots = l_NewSlots;
        ms_Capacity = l_Capacity + l_CapacityIncrease;

        LOW_LOG_DEBUG << "Auto-increased budget for Camera from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease)
                      << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Component
  }   // namespace Core
} // namespace Low
