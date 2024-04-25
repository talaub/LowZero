#include "LowCoreTransform.h"

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

      const uint16_t Transform::TYPE_ID = 25;
      uint32_t Transform::ms_Capacity = 0u;
      uint8_t *Transform::ms_Buffer = 0;
      Low::Util::Instances::Slot *Transform::ms_Slots = 0;
      Low::Util::List<Transform> Transform::ms_LivingInstances =
          Low::Util::List<Transform>();

      Transform::Transform() : Low::Util::Handle(0ull)
      {
      }
      Transform::Transform(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Transform::Transform(Transform &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle Transform::_make(Low::Util::Handle p_Entity)
      {
        Low::Core::Entity l_Entity = p_Entity.get_id();
        LOW_ASSERT(l_Entity.is_alive(),
                   "Cannot create component for dead entity");
        return make(l_Entity).get_id();
      }

      Transform Transform::make(Low::Core::Entity p_Entity)
      {
        uint32_t l_Index = create_instance();

        Transform l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Transform::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, position,
                                Low::Math::Vector3))
            Low::Math::Vector3();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, rotation,
                                Low::Math::Quaternion))
            Low::Math::Quaternion();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, scale,
                                Low::Math::Vector3))
            Low::Math::Vector3();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, children,
                                Low::Util::List<uint64_t>))
            Low::Util::List<uint64_t>();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, world_position,
                                Low::Math::Vector3))
            Low::Math::Vector3();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, world_rotation,
                                Low::Math::Quaternion))
            Low::Math::Quaternion();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, world_scale,
                                Low::Math::Vector3))
            Low::Math::Vector3();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, world_matrix,
                                Low::Math::Matrix4x4))
            Low::Math::Matrix4x4();
        ACCESSOR_TYPE_SOA(l_Handle, Transform, world_updated, bool) =
            false;
        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, entity,
                                Low::Core::Entity))
            Low::Core::Entity();
        ACCESSOR_TYPE_SOA(l_Handle, Transform, dirty, bool) = false;
        ACCESSOR_TYPE_SOA(l_Handle, Transform, world_dirty, bool) =
            false;

        l_Handle.set_entity(p_Entity);
        p_Entity.add_component(l_Handle);

        ms_LivingInstances.push_back(l_Handle);

        l_Handle.set_unique_id(
            Low::Util::generate_unique_id(l_Handle.get_id()));
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        l_Handle.scale(Math::Vector3(1.0f));
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Transform::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // Doing this to remove the transform from the list of
        // children
        set_parent(0);
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        Low::Util::remove_unique_id(get_unique_id());

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const Transform *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
      }

      void Transform::initialize()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Transform));

        initialize_buffer(&ms_Buffer, TransformData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_Transform);
        LOW_PROFILE_ALLOC(type_slots_Transform);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Transform);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Transform::is_alive;
        l_TypeInfo.destroy = &Transform::destroy;
        l_TypeInfo.serialize = &Transform::serialize;
        l_TypeInfo.deserialize = &Transform::deserialize;
        l_TypeInfo.make_default = nullptr;
        l_TypeInfo.make_component = &Transform::_make;
        l_TypeInfo.duplicate_default = nullptr;
        l_TypeInfo.duplicate_component = &Transform::_duplicate;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Transform::living_instances);
        l_TypeInfo.get_living_count = &Transform::living_count;
        l_TypeInfo.component = true;
        l_TypeInfo.uiComponent = false;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(position);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(TransformData, position);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.position();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Transform, position, Low::Math::Vector3);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.position(*(Low::Math::Vector3 *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(rotation);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(TransformData, rotation);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::QUATERNION;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.rotation();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Transform, rotation, Low::Math::Quaternion);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.rotation(*(Low::Math::Quaternion *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(scale);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(TransformData, scale);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.scale();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Transform, scale, Low::Math::Vector3);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.scale(*(Low::Math::Vector3 *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(parent);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(TransformData, parent);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.get_parent();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform,
                                              parent, uint64_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.set_parent(*(uint64_t *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(parent_uid);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(TransformData, parent_uid);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.get_parent_uid();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform,
                                              parent_uid, uint64_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(children);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(TransformData, children);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.get_children();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Transform, children,
                Low::Util::List<uint64_t>);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(world_position);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(TransformData, world_position);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.get_world_position();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform,
                                              world_position,
                                              Low::Math::Vector3);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(world_rotation);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(TransformData, world_rotation);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::QUATERNION;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.get_world_rotation();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform,
                                              world_rotation,
                                              Low::Math::Quaternion);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(world_scale);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(TransformData, world_scale);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.get_world_scale();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Transform, world_scale, Low::Math::Vector3);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(world_matrix);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(TransformData, world_matrix);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.get_world_matrix();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform,
                                              world_matrix,
                                              Low::Math::Matrix4x4);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(world_updated);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(TransformData, world_updated);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.is_world_updated();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform,
                                              world_updated, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.set_world_updated(*(bool *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(TransformData, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::TYPE_ID;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.get_entity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Transform, entity, Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(TransformData, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Transform, unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(dirty);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(TransformData, dirty);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.is_dirty();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform,
                                              dirty, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.set_dirty(*(bool *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(world_dirty);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(TransformData, world_dirty);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.is_world_dirty();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform,
                                              world_dirty, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Transform l_Handle = p_Handle.get_id();
            l_Handle.set_world_dirty(*(bool *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(recalculate_world_transform);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void Transform::cleanup()
      {
        Low::Util::List<Transform> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_Transform);
        LOW_PROFILE_FREE(type_slots_Transform);
      }

      Transform Transform::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Transform l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = Transform::TYPE_ID;

        return l_Handle;
      }

      bool Transform::is_alive() const
      {
        return m_Data.m_Type == Transform::TYPE_ID &&
               check_alive(ms_Slots, Transform::get_capacity());
      }

      uint32_t Transform::get_capacity()
      {
        return ms_Capacity;
      }

      Transform Transform::duplicate(Low::Core::Entity p_Entity) const
      {
        _LOW_ASSERT(is_alive());

        Transform l_Handle = make(p_Entity);
        l_Handle.position(position());
        l_Handle.rotation(rotation());
        l_Handle.scale(scale());
        l_Handle.set_dirty(is_dirty());
        l_Handle.set_world_dirty(is_world_dirty());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        return 0;
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      Transform Transform::duplicate(Transform p_Handle,
                                     Low::Core::Entity p_Entity)
      {
        return p_Handle.duplicate(p_Entity);
      }

      Low::Util::Handle
      Transform::_duplicate(Low::Util::Handle p_Handle,
                            Low::Util::Handle p_Entity)
      {
        Transform l_Transform = p_Handle.get_id();
        Low::Core::Entity l_Entity = p_Entity.get_id();
        return l_Transform.duplicate(l_Entity);
      }

      void Transform::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        Low::Util::Serialization::serialize(p_Node["position"],
                                            position());
        Low::Util::Serialization::serialize(p_Node["rotation"],
                                            rotation());
        Low::Util::Serialization::serialize(p_Node["scale"], scale());
        p_Node["parent_uid"] = get_parent_uid();
        p_Node["unique_id"] = get_unique_id();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Transform::serialize(Low::Util::Handle p_Handle,
                                Low::Util::Yaml::Node &p_Node)
      {
        Transform l_Transform = p_Handle.get_id();
        l_Transform.serialize(p_Node);
      }

      Low::Util::Handle
      Transform::deserialize(Low::Util::Yaml::Node &p_Node,
                             Low::Util::Handle p_Creator)
      {
        Transform l_Handle = Transform::make(p_Creator.get_id());

        if (p_Node["unique_id"]) {
          Low::Util::remove_unique_id(l_Handle.get_unique_id());
          l_Handle.set_unique_id(p_Node["unique_id"].as<uint64_t>());
          Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                        l_Handle.get_id());
        }

        if (p_Node["position"]) {
          l_Handle.position(
              Low::Util::Serialization::deserialize_vector3(
                  p_Node["position"]));
        }
        if (p_Node["rotation"]) {
          l_Handle.rotation(
              Low::Util::Serialization::deserialize_quaternion(
                  p_Node["rotation"]));
        }
        if (p_Node["scale"]) {
          l_Handle.scale(
              Low::Util::Serialization::deserialize_vector3(
                  p_Node["scale"]));
        }
        if (p_Node["parent_uid"]) {
          l_Handle.set_parent_uid(
              p_Node["parent_uid"].as<uint64_t>());
        }
        if (p_Node["unique_id"]) {
          l_Handle.set_unique_id(
              p_Node["unique_id"].as<Low::Util::UniqueId>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      Low::Math::Vector3 &Transform::position() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_position
        // LOW_CODEGEN::END::CUSTOM:GETTER_position

        return TYPE_SOA(Transform, position, Low::Math::Vector3);
      }
      void Transform::position(Low::Math::Vector3 &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_position
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_position

        if (position() != p_Value) {
          // Set dirty flags
          TYPE_SOA(Transform, dirty, bool) = true;
          TYPE_SOA(Transform, world_dirty, bool) = true;

          // Set new value
          TYPE_SOA(Transform, position, Low::Math::Vector3) = p_Value;
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
                    TYPE_ID, N(position),
                    !l_Prefab.compare_property(*this, N(position)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_position
          set_world_dirty(true);
          // LOW_CODEGEN::END::CUSTOM:SETTER_position
        }
      }

      Low::Math::Quaternion &Transform::rotation() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_rotation
        // LOW_CODEGEN::END::CUSTOM:GETTER_rotation

        return TYPE_SOA(Transform, rotation, Low::Math::Quaternion);
      }
      void Transform::rotation(Low::Math::Quaternion &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_rotation
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_rotation

        if (rotation() != p_Value) {
          // Set dirty flags
          TYPE_SOA(Transform, dirty, bool) = true;
          TYPE_SOA(Transform, world_dirty, bool) = true;

          // Set new value
          TYPE_SOA(Transform, rotation, Low::Math::Quaternion) =
              p_Value;
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
                    TYPE_ID, N(rotation),
                    !l_Prefab.compare_property(*this, N(rotation)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_rotation
          set_world_dirty(true);
          // LOW_CODEGEN::END::CUSTOM:SETTER_rotation
        }
      }

      Low::Math::Vector3 &Transform::scale() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_scale
        // LOW_CODEGEN::END::CUSTOM:GETTER_scale

        return TYPE_SOA(Transform, scale, Low::Math::Vector3);
      }
      void Transform::scale(Low::Math::Vector3 &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_scale
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_scale

        if (scale() != p_Value) {
          // Set dirty flags
          TYPE_SOA(Transform, dirty, bool) = true;
          TYPE_SOA(Transform, world_dirty, bool) = true;

          // Set new value
          TYPE_SOA(Transform, scale, Low::Math::Vector3) = p_Value;
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
                    TYPE_ID, N(scale),
                    !l_Prefab.compare_property(*this, N(scale)));
              }
            }
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_scale
          set_world_dirty(true);
          // LOW_CODEGEN::END::CUSTOM:SETTER_scale
        }
      }

      uint64_t Transform::get_parent() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_parent
        // LOW_CODEGEN::END::CUSTOM:GETTER_parent

        return TYPE_SOA(Transform, parent, uint64_t);
      }
      void Transform::set_parent(uint64_t p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_parent
        Transform l_Parent = get_parent();
        if (l_Parent.is_alive()) {
          for (auto it = l_Parent.get_children().begin();
               it != l_Parent.get_children().end();) {
            if (*it == get_id()) {
              it = l_Parent.get_children().erase(it);
            } else {
              ++it;
            }
          }
        }
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_parent

        if (get_parent() != p_Value) {
          // Set dirty flags
          TYPE_SOA(Transform, dirty, bool) = true;
          TYPE_SOA(Transform, world_dirty, bool) = true;

          // Set new value
          TYPE_SOA(Transform, parent, uint64_t) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_parent
          Transform l_Parent(p_Value);
          if (l_Parent.is_alive()) {
            set_parent_uid(l_Parent.get_unique_id());
            l_Parent.get_children().push_back(get_id());
          } else {
            set_parent_uid(0);
          }
          // LOW_CODEGEN::END::CUSTOM:SETTER_parent
        }
      }

      uint64_t Transform::get_parent_uid() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_parent_uid
        // LOW_CODEGEN::END::CUSTOM:GETTER_parent_uid

        return TYPE_SOA(Transform, parent_uid, uint64_t);
      }
      void Transform::set_parent_uid(uint64_t p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_parent_uid
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_parent_uid

        if (get_parent_uid() != p_Value) {
          // Set dirty flags
          TYPE_SOA(Transform, dirty, bool) = true;
          TYPE_SOA(Transform, world_dirty, bool) = true;

          // Set new value
          TYPE_SOA(Transform, parent_uid, uint64_t) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_parent_uid
          // LOW_CODEGEN::END::CUSTOM:SETTER_parent_uid
        }
      }

      Low::Util::List<uint64_t> &Transform::get_children() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_children
        // LOW_CODEGEN::END::CUSTOM:GETTER_children

        return TYPE_SOA(Transform, children,
                        Low::Util::List<uint64_t>);
      }

      Low::Math::Vector3 &Transform::get_world_position()
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_position
        recalculate_world_transform();
        // LOW_CODEGEN::END::CUSTOM:GETTER_world_position

        return TYPE_SOA(Transform, world_position,
                        Low::Math::Vector3);
      }
      void Transform::set_world_position(Low::Math::Vector3 &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_position
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_position

        // Set new value
        TYPE_SOA(Transform, world_position, Low::Math::Vector3) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_position
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_position
      }

      Low::Math::Quaternion &Transform::get_world_rotation()
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_rotation
        recalculate_world_transform();
        // LOW_CODEGEN::END::CUSTOM:GETTER_world_rotation

        return TYPE_SOA(Transform, world_rotation,
                        Low::Math::Quaternion);
      }
      void
      Transform::set_world_rotation(Low::Math::Quaternion &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_rotation
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_rotation

        // Set new value
        TYPE_SOA(Transform, world_rotation, Low::Math::Quaternion) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_rotation
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_rotation
      }

      Low::Math::Vector3 &Transform::get_world_scale()
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_scale
        recalculate_world_transform();
        // LOW_CODEGEN::END::CUSTOM:GETTER_world_scale

        return TYPE_SOA(Transform, world_scale, Low::Math::Vector3);
      }
      void Transform::set_world_scale(Low::Math::Vector3 &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_scale
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_scale

        // Set new value
        TYPE_SOA(Transform, world_scale, Low::Math::Vector3) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_scale
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_scale
      }

      Low::Math::Matrix4x4 &Transform::get_world_matrix()
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_matrix
        recalculate_world_transform();
        // LOW_CODEGEN::END::CUSTOM:GETTER_world_matrix

        return TYPE_SOA(Transform, world_matrix,
                        Low::Math::Matrix4x4);
      }
      void Transform::set_world_matrix(Low::Math::Matrix4x4 &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_matrix
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_matrix

        // Set new value
        TYPE_SOA(Transform, world_matrix, Low::Math::Matrix4x4) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_matrix
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_matrix
      }

      bool Transform::is_world_updated() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_updated
        // LOW_CODEGEN::END::CUSTOM:GETTER_world_updated

        return TYPE_SOA(Transform, world_updated, bool);
      }
      void Transform::set_world_updated(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_updated
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_updated

        // Set new value
        TYPE_SOA(Transform, world_updated, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_updated
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_updated
      }

      Low::Core::Entity Transform::get_entity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity
        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        return TYPE_SOA(Transform, entity, Low::Core::Entity);
      }
      void Transform::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

        // Set new value
        TYPE_SOA(Transform, entity, Low::Core::Entity) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity
        // LOW_CODEGEN::END::CUSTOM:SETTER_entity
      }

      Low::Util::UniqueId Transform::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(Transform, unique_id, Low::Util::UniqueId);
      }
      void Transform::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(Transform, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
      }

      bool Transform::is_dirty() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

        return TYPE_SOA(Transform, dirty, bool);
      }
      void Transform::set_dirty(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

        // Set new value
        TYPE_SOA(Transform, dirty, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:SETTER_dirty
      }

      bool Transform::is_world_dirty() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_dirty
        if (TYPE_SOA(Transform, world_dirty, bool)) {
          return TYPE_SOA(Transform, world_dirty, bool);
        }

        Transform l_Parent = get_parent();

        if (l_Parent.is_alive()) {
          return l_Parent.is_world_dirty() ||
                 l_Parent.is_world_updated();
        }
        // LOW_CODEGEN::END::CUSTOM:GETTER_world_dirty

        return TYPE_SOA(Transform, world_dirty, bool);
      }
      void Transform::set_world_dirty(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_dirty
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_dirty

        // Set new value
        TYPE_SOA(Transform, world_dirty, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_dirty
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_dirty
      }

      void Transform::recalculate_world_transform()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_recalculate_world_transform
        LOW_ASSERT(is_alive(), "Cannot calculate world "
                               "position of dead transform");

        if (!is_world_dirty() || is_world_updated()) {
          return;
        }

        if (!Transform(get_parent()).is_alive() &&
            get_parent_uid() != 0) {
          set_parent(Util::find_handle_by_unique_id(get_parent_uid())
                         .get_id());
        }

        Low::Math::Vector3 l_Position = position();
        Low::Math::Quaternion l_Rotation = rotation();
        Low::Math::Vector3 l_Scale = scale();

        Transform l_Parent = get_parent();

        Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

        l_LocalMatrix = glm::translate(l_LocalMatrix, l_Position);
        l_LocalMatrix *= glm::toMat4(l_Rotation);
        l_LocalMatrix = glm::scale(l_LocalMatrix, l_Scale);

        if (l_Parent.is_alive()) {
          if (l_Parent.is_world_dirty()) {
            l_Parent.recalculate_world_transform();
          }

          Math::Matrix4x4 l_ParentMatrix =
              l_Parent.get_world_matrix();

          Low::Math::Matrix4x4 l_WorldMatrix =
              l_ParentMatrix * l_LocalMatrix;

          Low::Math::Vector3 l_WorldScale;
          Low::Math::Quaternion l_WorldRotation;
          Low::Math::Vector3 l_WorldPosition;
          Low::Math::Vector3 l_WorldSkew;
          Low::Math::Vector4 l_WorldPerspective;

          set_world_matrix(l_WorldMatrix);

          glm::decompose(l_WorldMatrix, l_WorldScale, l_WorldRotation,
                         l_WorldPosition, l_WorldSkew,
                         l_WorldPerspective);

          l_Position = l_WorldPosition;
          l_Rotation = l_WorldRotation;
          l_Scale = l_WorldScale;
        } else {
          set_world_matrix(l_LocalMatrix);
        }

        set_world_position(l_Position);
        set_world_rotation(l_Rotation);
        set_world_scale(l_Scale);

        set_world_dirty(false);
        set_world_updated(true);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_recalculate_world_transform
      }

      uint32_t Transform::create_instance()
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

      void Transform::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease =
            std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0,
                   "Could not increase capacity");

        uint8_t *l_NewBuffer =
            (uint8_t *)malloc((l_Capacity + l_CapacityIncrease) *
                              sizeof(TransformData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, position) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, position) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Math::Vector3));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, rotation) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, rotation) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Math::Quaternion));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, scale) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, scale) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Math::Vector3));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, parent) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, parent) *
                            (l_Capacity)],
                 l_Capacity * sizeof(uint64_t));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, parent_uid) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, parent_uid) *
                            (l_Capacity)],
                 l_Capacity * sizeof(uint64_t));
        }
        {
          for (auto it = ms_LivingInstances.begin();
               it != ms_LivingInstances.end(); ++it) {
            auto *i_ValPtr = new (
                &l_NewBuffer[offsetof(TransformData, children) *
                                 (l_Capacity + l_CapacityIncrease) +
                             (it->get_index() *
                              sizeof(Low::Util::List<uint64_t>))])
                Low::Util::List<uint64_t>();
            *i_ValPtr = it->get_children();
          }
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(TransformData, world_position) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(TransformData, world_position) *
                         (l_Capacity)],
              l_Capacity * sizeof(Low::Math::Vector3));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(TransformData, world_rotation) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(TransformData, world_rotation) *
                         (l_Capacity)],
              l_Capacity * sizeof(Low::Math::Quaternion));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, world_scale) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, world_scale) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Math::Vector3));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, world_matrix) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, world_matrix) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Math::Matrix4x4));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, world_updated) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, world_updated) *
                            (l_Capacity)],
                 l_Capacity * sizeof(bool));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, entity) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, entity) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Core::Entity));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, unique_id) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, unique_id) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Util::UniqueId));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, dirty) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, dirty) *
                            (l_Capacity)],
                 l_Capacity * sizeof(bool));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, world_dirty) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, world_dirty) *
                            (l_Capacity)],
                 l_Capacity * sizeof(bool));
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

        LOW_LOG_DEBUG << "Auto-increased budget for Transform from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease)
                      << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Component
  }   // namespace Core
} // namespace Low
