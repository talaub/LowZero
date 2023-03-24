#include "LowRendererRenderObject.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Renderer {
    const uint16_t RenderObject::TYPE_ID = 8;
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
      uint32_t l_Index = Low::Util::Instances::create_instance(
          ms_Buffer, ms_Slots, get_capacity());

      RenderObjectData *l_DataPtr =
          (RenderObjectData *)&ms_Buffer[l_Index * sizeof(RenderObjectData)];
      new (l_DataPtr) RenderObjectData();

      RenderObject l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = RenderObject::TYPE_ID;

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
      initialize_buffer(&ms_Buffer, RenderObjectData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_RenderObject);
      LOW_PROFILE_ALLOC(type_slots_RenderObject);
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
      static uint32_t l_Capacity = 0u;
      if (l_Capacity == 0u) {
        l_Capacity =
            Low::Util::Config::get_capacity(N(LowRenderer), N(RenderObject));
      }
      return l_Capacity;
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

  } // namespace Renderer
} // namespace Low
