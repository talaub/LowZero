#include "LowRendererMesh.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Renderer {
    const uint16_t Mesh::TYPE_ID = 8;
    uint8_t *Mesh::ms_Buffer = 0;
    Low::Util::Instances::Slot *Mesh::ms_Slots = 0;
    Low::Util::List<Mesh> Mesh::ms_LivingInstances = Low::Util::List<Mesh>();

    Mesh::Mesh() : Low::Util::Handle(0ull)
    {
    }
    Mesh::Mesh(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Mesh::Mesh(Mesh &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Mesh Mesh::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = Low::Util::Instances::create_instance(
          ms_Buffer, ms_Slots, get_capacity());

      Mesh l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Mesh::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, Mesh, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      return l_Handle;
    }

    void Mesh::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const Mesh *l_Instances = living_instances();
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

    void Mesh::initialize()
    {
      initialize_buffer(&ms_Buffer, MeshData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_Mesh);
      LOW_PROFILE_ALLOC(type_slots_Mesh);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Mesh);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Mesh::is_alive;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertex_buffer_start);
        l_PropertyInfo.dataOffset = offsetof(MeshData, vertex_buffer_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh, vertex_buffer_start,
                                            uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, Mesh, vertex_buffer_start, uint32_t) =
              *(uint32_t *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertex_count);
        l_PropertyInfo.dataOffset = offsetof(MeshData, vertex_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh, vertex_count,
                                            uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, Mesh, vertex_count, uint32_t) =
              *(uint32_t *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(index_buffer_start);
        l_PropertyInfo.dataOffset = offsetof(MeshData, index_buffer_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh, index_buffer_start,
                                            uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, Mesh, index_buffer_start, uint32_t) =
              *(uint32_t *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(index_count);
        l_PropertyInfo.dataOffset = offsetof(MeshData, index_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh, index_count,
                                            uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, Mesh, index_count, uint32_t) =
              *(uint32_t *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.dataOffset = offsetof(MeshData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, Mesh, name, Low::Util::Name) =
              *(Low::Util::Name *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Mesh::cleanup()
    {
      Low::Util::List<Mesh> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Mesh);
      LOW_PROFILE_FREE(type_slots_Mesh);
    }

    bool Mesh::is_alive() const
    {
      return m_Data.m_Type == Mesh::TYPE_ID &&
             check_alive(ms_Slots, Mesh::get_capacity());
    }

    uint32_t Mesh::get_capacity()
    {
      static uint32_t l_Capacity = 0u;
      if (l_Capacity == 0u) {
        l_Capacity = Low::Util::Config::get_capacity(N(LowRenderer), N(Mesh));
      }
      return l_Capacity;
    }

    uint32_t Mesh::get_vertex_buffer_start() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Mesh, vertex_buffer_start, uint32_t);
    }
    void Mesh::set_vertex_buffer_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(Mesh, vertex_buffer_start, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertex_buffer_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_vertex_buffer_start
    }

    uint32_t Mesh::get_vertex_count() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Mesh, vertex_count, uint32_t);
    }
    void Mesh::set_vertex_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(Mesh, vertex_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_vertex_count
    }

    uint32_t Mesh::get_index_buffer_start() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Mesh, index_buffer_start, uint32_t);
    }
    void Mesh::set_index_buffer_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(Mesh, index_buffer_start, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_index_buffer_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_index_buffer_start
    }

    uint32_t Mesh::get_index_count() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Mesh, index_count, uint32_t);
    }
    void Mesh::set_index_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(Mesh, index_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_index_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_index_count
    }

    Low::Util::Name Mesh::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Mesh, name, Low::Util::Name);
    }
    void Mesh::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(Mesh, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

  } // namespace Renderer
} // namespace Low
