#include "LowCoreTransform.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Core {
    namespace Component {
      const uint16_t Transform::TYPE_ID = 21;
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
      Transform::Transform(Transform &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Transform Transform::make(Low::Core::Entity p_Entity)
      {
        uint32_t l_Index = create_instance();

        Transform l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Transform::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, position, Math::Vector3))
            Math::Vector3();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, rotation,
                                Math::Quaternion)) Math::Quaternion();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, scale, Math::Vector3))
            Math::Vector3();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, world_position,
                                Math::Vector3)) Math::Vector3();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, world_rotation,
                                Math::Quaternion)) Math::Quaternion();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, world_scale,
                                Math::Vector3)) Math::Vector3();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Transform, entity, Low::Core::Entity))
            Low::Core::Entity();
        ACCESSOR_TYPE_SOA(l_Handle, Transform, dirty, bool) = false;
        ACCESSOR_TYPE_SOA(l_Handle, Transform, world_dirty, bool) = false;

        l_Handle.set_entity(p_Entity);
        p_Entity.add_component(l_Handle);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void Transform::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // LOW_CODEGEN::END::CUSTOM:DESTROY

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
        _LOW_ASSERT(l_LivingInstanceFound);
      }

      void Transform::initialize()
      {
        ms_Capacity = Low::Util::Config::get_capacity(N(LowCore), N(Transform));

        initialize_buffer(&ms_Buffer, TransformData::get_size(), get_capacity(),
                          &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_Transform);
        LOW_PROFILE_ALLOC(type_slots_Transform);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Transform);
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Transform::is_alive;
        l_TypeInfo.destroy = &Transform::destroy;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Transform::living_instances);
        l_TypeInfo.get_living_count = &Transform::living_count;
        l_TypeInfo.component = true;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(position);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(TransformData, position);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform, position,
                                              Math::Vector3);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Transform, position, Math::Vector3) =
                *(Math::Vector3 *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(rotation);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(TransformData, rotation);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::QUATERNION;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform, rotation,
                                              Math::Quaternion);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Transform, rotation, Math::Quaternion) =
                *(Math::Quaternion *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(scale);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(TransformData, scale);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform, scale,
                                              Math::Vector3);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Transform, scale, Math::Vector3) =
                *(Math::Vector3 *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(parent);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(TransformData, parent);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform, parent,
                                              uint64_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Transform, parent, uint64_t) =
                *(uint64_t *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(world_position);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(TransformData, world_position);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform,
                                              world_position, Math::Vector3);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Transform, world_position,
                              Math::Vector3) = *(Math::Vector3 *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(world_rotation);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(TransformData, world_rotation);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::QUATERNION;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform,
                                              world_rotation, Math::Quaternion);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Transform, world_rotation,
                              Math::Quaternion) = *(Math::Quaternion *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(world_scale);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(TransformData, world_scale);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform, world_scale,
                                              Math::Vector3);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Transform, world_scale, Math::Vector3) =
                *(Math::Vector3 *)p_Data;
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
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform, entity,
                                              Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Transform, entity, Low::Core::Entity) =
                *(Low::Core::Entity *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(dirty);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(TransformData, dirty);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform, dirty, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Transform, dirty, bool) =
                *(bool *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(world_dirty);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(TransformData, world_dirty);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Transform, world_dirty,
                                              bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Transform, world_dirty, bool) =
                *(bool *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
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

      void Transform::serialize(Low::Util::Yaml::Node &p_Node) const
      {
      }

      Math::Vector3 &Transform::position() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Transform, position, Math::Vector3);
      }
      void Transform::position(Math::Vector3 &p_Value)
      {
        _LOW_ASSERT(is_alive());

        if (position() != p_Value) {
          // Set dirty flags
          TYPE_SOA(Transform, dirty, bool) = true;
          TYPE_SOA(Transform, world_dirty, bool) = true;

          // Set new value
          TYPE_SOA(Transform, position, Math::Vector3) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_position
          // LOW_CODEGEN::END::CUSTOM:SETTER_position
        }
      }

      Math::Quaternion &Transform::rotation() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Transform, rotation, Math::Quaternion);
      }
      void Transform::rotation(Math::Quaternion &p_Value)
      {
        _LOW_ASSERT(is_alive());

        if (rotation() != p_Value) {
          // Set dirty flags
          TYPE_SOA(Transform, dirty, bool) = true;
          TYPE_SOA(Transform, world_dirty, bool) = true;

          // Set new value
          TYPE_SOA(Transform, rotation, Math::Quaternion) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_rotation
          // LOW_CODEGEN::END::CUSTOM:SETTER_rotation
        }
      }

      Math::Vector3 &Transform::scale() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Transform, scale, Math::Vector3);
      }
      void Transform::scale(Math::Vector3 &p_Value)
      {
        _LOW_ASSERT(is_alive());

        if (scale() != p_Value) {
          // Set dirty flags
          TYPE_SOA(Transform, dirty, bool) = true;
          TYPE_SOA(Transform, world_dirty, bool) = true;

          // Set new value
          TYPE_SOA(Transform, scale, Math::Vector3) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_scale
          // LOW_CODEGEN::END::CUSTOM:SETTER_scale
        }
      }

      uint64_t Transform::get_parent() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Transform, parent, uint64_t);
      }
      void Transform::set_parent(uint64_t p_Value)
      {
        _LOW_ASSERT(is_alive());

        if (get_parent() != p_Value) {
          // Set dirty flags
          TYPE_SOA(Transform, dirty, bool) = true;
          TYPE_SOA(Transform, world_dirty, bool) = true;

          // Set new value
          TYPE_SOA(Transform, parent, uint64_t) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_parent
          // LOW_CODEGEN::END::CUSTOM:SETTER_parent
        }
      }

      Math::Vector3 &Transform::get_world_position() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Transform, world_position, Math::Vector3);
      }
      void Transform::set_world_position(Math::Vector3 &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Transform, world_position, Math::Vector3) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_position
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_position
      }

      Math::Quaternion &Transform::get_world_rotation() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Transform, world_rotation, Math::Quaternion);
      }
      void Transform::set_world_rotation(Math::Quaternion &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Transform, world_rotation, Math::Quaternion) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_rotation
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_rotation
      }

      Math::Vector3 &Transform::get_world_scale() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Transform, world_scale, Math::Vector3);
      }
      void Transform::set_world_scale(Math::Vector3 &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Transform, world_scale, Math::Vector3) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_scale
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_scale
      }

      Low::Core::Entity Transform::get_entity() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Transform, entity, Low::Core::Entity);
      }
      void Transform::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Transform, entity, Low::Core::Entity) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity
        // LOW_CODEGEN::END::CUSTOM:SETTER_entity
      }

      bool Transform::is_dirty() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Transform, dirty, bool);
      }
      void Transform::set_dirty(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Transform, dirty, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:SETTER_dirty
      }

      bool Transform::is_world_dirty() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Transform, world_dirty, bool);
      }
      void Transform::set_world_dirty(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Transform, world_dirty, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_dirty
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_dirty
      }

      void Transform::recalculate_world_transform()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_recalculate_world_transform
        LOW_ASSERT(is_alive(),
                   "Cannot calculate world position of dead transform");

        Low::Math::Vector3 l_Position = position();
        Low::Math::Quaternion l_Rotation = rotation();
        Low::Math::Vector3 l_Scale = scale();

        Transform l_Parent = get_parent();

        if (l_Parent.is_alive()) {
          if (l_Parent.is_world_dirty()) {
            l_Parent.recalculate_world_transform();
          }

          Low::Math::Vector3 l_ParentPosition = l_Parent.get_world_position();
          Low::Math::Quaternion l_ParentRotation =
              l_Parent.get_world_rotation();
          Low::Math::Vector3 l_ParentScale = l_Parent.get_world_scale();

          Low::Math::Matrix4x4 l_ParentMatrix(1.0f);

          l_ParentMatrix = glm::translate(l_ParentMatrix, l_ParentPosition);
          l_ParentMatrix *= glm::toMat4(l_ParentRotation);
          l_ParentMatrix = glm::scale(l_ParentMatrix, l_ParentScale);

          Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

          l_LocalMatrix = glm::translate(l_LocalMatrix, l_Position);
          l_LocalMatrix *= glm::toMat4(l_Rotation);
          l_LocalMatrix = glm::scale(l_LocalMatrix, l_Scale);

          Low::Math::Matrix4x4 l_WorldMatrix = l_ParentMatrix * l_LocalMatrix;

          Low::Math::Vector3 l_WorldScale;
          Low::Math::Quaternion l_WorldRotation;
          Low::Math::Vector3 l_WorldPosition;
          Low::Math::Vector3 l_WorldSkew;
          Low::Math::Vector4 l_WorldPerspective;

          glm::decompose(l_WorldMatrix, l_WorldScale, l_WorldRotation,
                         l_WorldPosition, l_WorldSkew, l_WorldPerspective);

          l_Position = l_WorldPosition;
          l_Rotation = l_WorldRotation;
          l_Scale = l_WorldScale;
        }

        set_world_position(l_Position);
        set_world_rotation(l_Rotation);
        set_world_scale(l_Scale);

        set_world_dirty(false);
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
        uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

        uint8_t *l_NewBuffer = (uint8_t *)malloc(
            (l_Capacity + l_CapacityIncrease) * sizeof(TransformData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, position) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, position) * (l_Capacity)],
                 l_Capacity * sizeof(Math::Vector3));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, rotation) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, rotation) * (l_Capacity)],
                 l_Capacity * sizeof(Math::Quaternion));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, scale) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, scale) * (l_Capacity)],
                 l_Capacity * sizeof(Math::Vector3));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, parent) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, parent) * (l_Capacity)],
                 l_Capacity * sizeof(uint64_t));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, world_position) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, world_position) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Math::Vector3));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, world_rotation) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, world_rotation) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Math::Quaternion));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(TransformData, world_scale) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(TransformData, world_scale) * (l_Capacity)],
              l_Capacity * sizeof(Math::Vector3));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, entity) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, entity) * (l_Capacity)],
                 l_Capacity * sizeof(Low::Core::Entity));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TransformData, dirty) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TransformData, dirty) * (l_Capacity)],
                 l_Capacity * sizeof(bool));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(TransformData, world_dirty) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(TransformData, world_dirty) * (l_Capacity)],
              l_Capacity * sizeof(bool));
        }
        for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease;
             ++i) {
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
                      << (l_Capacity + l_CapacityIncrease) << LOW_LOG_END;
      }
    } // namespace Component
  }   // namespace Core
} // namespace Low
