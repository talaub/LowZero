#include "LowRendererRenderObject.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Renderer {
    const uint16_t RenderObject::TYPE_ID = 9;
    uint32_t RenderObject::ms_Capacity = 0u;
    uint8_t *RenderObject::ms_Buffer = 0;
    Low::Util::Instances::Slot *RenderObject::ms_Slots = 0;
    Low::Util::List<RenderObject> RenderObject::ms_LivingInstances =
        Low::Util::List<RenderObject>();

    RenderObject::RenderObject() : Low::Util::Handle(0ull)
    {
    }
    RenderObject::RenderObject(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    RenderObject::RenderObject(RenderObject &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    RenderObject RenderObject::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = create_instance();

      RenderObject l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = RenderObject::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderObject, mesh, Mesh)) Mesh();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderObject, material, Material))
          Material();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderObject, world_position,
                              Math::Vector3)) Math::Vector3();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderObject, world_rotation,
                              Math::Quaternion)) Math::Quaternion();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderObject, world_scale,
                              Math::Vector3)) Math::Vector3();
      ACCESSOR_TYPE_SOA(l_Handle, RenderObject, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      return l_Handle;
    }

    void RenderObject::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const RenderObject *l_Instances = living_instances();
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

    void RenderObject::initialize()
    {
      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowRenderer), N(RenderObject));

      initialize_buffer(&ms_Buffer, RenderObjectData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_RenderObject);
      LOW_PROFILE_ALLOC(type_slots_RenderObject);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(RenderObject);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &RenderObject::is_alive;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(mesh);
        l_PropertyInfo.dataOffset = offsetof(RenderObjectData, mesh);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject, mesh, Mesh);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, RenderObject, mesh, Mesh) =
              *(Mesh *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(material);
        l_PropertyInfo.dataOffset = offsetof(RenderObjectData, material);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject, material,
                                            Material);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, RenderObject, material, Material) =
              *(Material *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(world_position);
        l_PropertyInfo.dataOffset = offsetof(RenderObjectData, world_position);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            world_position, Math::Vector3);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, RenderObject, world_position,
                            Math::Vector3) = *(Math::Vector3 *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(world_rotation);
        l_PropertyInfo.dataOffset = offsetof(RenderObjectData, world_rotation);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            world_rotation, Math::Quaternion);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, RenderObject, world_rotation,
                            Math::Quaternion) = *(Math::Quaternion *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(world_scale);
        l_PropertyInfo.dataOffset = offsetof(RenderObjectData, world_scale);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject, world_scale,
                                            Math::Vector3);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, RenderObject, world_scale,
                            Math::Vector3) = *(Math::Vector3 *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.dataOffset = offsetof(RenderObjectData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, RenderObject, name, Low::Util::Name) =
              *(Low::Util::Name *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void RenderObject::cleanup()
    {
      Low::Util::List<RenderObject> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_RenderObject);
      LOW_PROFILE_FREE(type_slots_RenderObject);
    }

    bool RenderObject::is_alive() const
    {
      return m_Data.m_Type == RenderObject::TYPE_ID &&
             check_alive(ms_Slots, RenderObject::get_capacity());
    }

    uint32_t RenderObject::get_capacity()
    {
      return ms_Capacity;
    }

    Mesh RenderObject::get_mesh() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderObject, mesh, Mesh);
    }
    void RenderObject::set_mesh(Mesh p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderObject, mesh, Mesh) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mesh
      // LOW_CODEGEN::END::CUSTOM:SETTER_mesh
    }

    Material RenderObject::get_material() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderObject, material, Material);
    }
    void RenderObject::set_material(Material p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderObject, material, Material) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material
      // LOW_CODEGEN::END::CUSTOM:SETTER_material
    }

    Math::Vector3 &RenderObject::get_world_position() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderObject, world_position, Math::Vector3);
    }
    void RenderObject::set_world_position(Math::Vector3 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderObject, world_position, Math::Vector3) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_position
      // LOW_CODEGEN::END::CUSTOM:SETTER_world_position
    }

    Math::Quaternion &RenderObject::get_world_rotation() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderObject, world_rotation, Math::Quaternion);
    }
    void RenderObject::set_world_rotation(Math::Quaternion &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderObject, world_rotation, Math::Quaternion) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_rotation
      // LOW_CODEGEN::END::CUSTOM:SETTER_world_rotation
    }

    Math::Vector3 &RenderObject::get_world_scale() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderObject, world_scale, Math::Vector3);
    }
    void RenderObject::set_world_scale(Math::Vector3 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderObject, world_scale, Math::Vector3) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_scale
      // LOW_CODEGEN::END::CUSTOM:SETTER_world_scale
    }

    Low::Util::Name RenderObject::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderObject, name, Low::Util::Name);
    }
    void RenderObject::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderObject, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    uint32_t RenderObject::create_instance()
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

    void RenderObject::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(RenderObjectData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(RenderObjectData, mesh) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderObjectData, mesh) * (l_Capacity)],
               l_Capacity * sizeof(Mesh));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderObjectData, material) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderObjectData, material) * (l_Capacity)],
               l_Capacity * sizeof(Material));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderObjectData, world_position) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderObjectData, world_position) *
                          (l_Capacity)],
               l_Capacity * sizeof(Math::Vector3));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderObjectData, world_rotation) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderObjectData, world_rotation) *
                          (l_Capacity)],
               l_Capacity * sizeof(Math::Quaternion));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(RenderObjectData, world_scale) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(RenderObjectData, world_scale) * (l_Capacity)],
            l_Capacity * sizeof(Math::Vector3));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderObjectData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderObjectData, name) * (l_Capacity)],
               l_Capacity * sizeof(Low::Util::Name));
      }
      for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease; ++i) {
        l_NewSlots[i].m_Occupied = false;
        l_NewSlots[i].m_Generation = 0;
      }
      free(ms_Buffer);
      free(ms_Slots);
      ms_Buffer = l_NewBuffer;
      ms_Slots = l_NewSlots;
      ms_Capacity = l_Capacity + l_CapacityIncrease;

      LOW_LOG_DEBUG << "Auto-increased budget for RenderObject from "
                    << l_Capacity << " to " << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }
  } // namespace Renderer
} // namespace Low
