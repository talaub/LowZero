#include "LowRendererGpuSubmesh.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t GpuSubmesh::TYPE_ID = 69;
    uint32_t GpuSubmesh::ms_Capacity = 0u;
    uint8_t *GpuSubmesh::ms_Buffer = 0;
    std::shared_mutex GpuSubmesh::ms_BufferMutex;
    Low::Util::Instances::Slot *GpuSubmesh::ms_Slots = 0;
    Low::Util::List<GpuSubmesh> GpuSubmesh::ms_LivingInstances =
        Low::Util::List<GpuSubmesh>();

    GpuSubmesh::GpuSubmesh() : Low::Util::Handle(0ull)
    {
    }
    GpuSubmesh::GpuSubmesh(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    GpuSubmesh::GpuSubmesh(GpuSubmesh &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle GpuSubmesh::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    GpuSubmesh GpuSubmesh::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      GpuSubmesh l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = GpuSubmesh::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, GpuSubmesh, state, MeshState))
          MeshState();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GpuSubmesh, transform,
                              Low::Math::Matrix4x4))
          Low::Math::Matrix4x4();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GpuSubmesh, parent_transform,
                              Low::Math::Matrix4x4))
          Low::Math::Matrix4x4();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GpuSubmesh, local_transform,
                              Low::Math::Matrix4x4))
          Low::Math::Matrix4x4();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GpuSubmesh, aabb,
                              Low::Math::AABB)) Low::Math::AABB();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GpuSubmesh, bounding_sphere,
                              Low::Math::Sphere)) Low::Math::Sphere();
      ACCESSOR_TYPE_SOA(l_Handle, GpuSubmesh, name, Low::Util::Name) =
          Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void GpuSubmesh::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      broadcast_observable(OBSERVABLE_DESTROY);

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end();) {
        if (it->get_id() == get_id()) {
          it = ms_LivingInstances.erase(it);
        } else {
          it++;
        }
      }
    }

    void GpuSubmesh::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(GpuSubmesh));

      initialize_buffer(&ms_Buffer, GpuSubmeshData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_GpuSubmesh);
      LOW_PROFILE_ALLOC(type_slots_GpuSubmesh);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(GpuSubmesh);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &GpuSubmesh::is_alive;
      l_TypeInfo.destroy = &GpuSubmesh::destroy;
      l_TypeInfo.serialize = &GpuSubmesh::serialize;
      l_TypeInfo.deserialize = &GpuSubmesh::deserialize;
      l_TypeInfo.find_by_index = &GpuSubmesh::_find_by_index;
      l_TypeInfo.notify = &GpuSubmesh::_notify;
      l_TypeInfo.find_by_name = &GpuSubmesh::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &GpuSubmesh::_make;
      l_TypeInfo.duplicate_default = &GpuSubmesh::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &GpuSubmesh::living_instances);
      l_TypeInfo.get_living_count = &GpuSubmesh::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(GpuSubmeshData, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            MeshStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuSubmesh,
                                            state, MeshState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(MeshState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuSubmesh l_Handle = p_Handle.get_id();
          *((MeshState *)p_Data) = l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
      {
        // Property: uploaded_vertex_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uploaded_vertex_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GpuSubmeshData, uploaded_vertex_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.get_uploaded_vertex_count();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GpuSubmesh, uploaded_vertex_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.set_uploaded_vertex_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuSubmesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) =
              l_Handle.get_uploaded_vertex_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: uploaded_vertex_count
      }
      {
        // Property: uploaded_index_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uploaded_index_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GpuSubmeshData, uploaded_index_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.get_uploaded_index_count();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GpuSubmesh, uploaded_index_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.set_uploaded_index_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuSubmesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_uploaded_index_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: uploaded_index_count
      }
      {
        // Property: vertex_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertex_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GpuSubmeshData, vertex_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.get_vertex_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuSubmesh,
                                            vertex_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.set_vertex_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuSubmesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_vertex_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: vertex_count
      }
      {
        // Property: index_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(index_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GpuSubmeshData, index_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.get_index_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuSubmesh,
                                            index_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.set_index_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuSubmesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_index_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: index_count
      }
      {
        // Property: vertex_start
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertex_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GpuSubmeshData, vertex_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.get_vertex_start();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuSubmesh,
                                            vertex_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.set_vertex_start(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuSubmesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_vertex_start();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: vertex_start
      }
      {
        // Property: index_start
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(index_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GpuSubmeshData, index_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.get_index_start();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuSubmesh,
                                            index_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.set_index_start(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuSubmesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_index_start();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: index_start
      }
      {
        // Property: transform
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(transform);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GpuSubmeshData, transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.get_transform();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GpuSubmesh, transform, Low::Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.set_transform(*(Low::Math::Matrix4x4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuSubmesh l_Handle = p_Handle.get_id();
          *((Low::Math::Matrix4x4 *)p_Data) =
              l_Handle.get_transform();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: transform
      }
      {
        // Property: parent_transform
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(parent_transform);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GpuSubmeshData, parent_transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.get_parent_transform();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuSubmesh,
                                            parent_transform,
                                            Low::Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.set_parent_transform(
              *(Low::Math::Matrix4x4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuSubmesh l_Handle = p_Handle.get_id();
          *((Low::Math::Matrix4x4 *)p_Data) =
              l_Handle.get_parent_transform();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: parent_transform
      }
      {
        // Property: local_transform
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(local_transform);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GpuSubmeshData, local_transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.get_local_transform();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuSubmesh,
                                            local_transform,
                                            Low::Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.set_local_transform(
              *(Low::Math::Matrix4x4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuSubmesh l_Handle = p_Handle.get_id();
          *((Low::Math::Matrix4x4 *)p_Data) =
              l_Handle.get_local_transform();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: local_transform
      }
      {
        // Property: aabb
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(aabb);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(GpuSubmeshData, aabb);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.get_aabb();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuSubmesh,
                                            aabb, Low::Math::AABB);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.set_aabb(*(Low::Math::AABB *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuSubmesh l_Handle = p_Handle.get_id();
          *((Low::Math::AABB *)p_Data) = l_Handle.get_aabb();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: aabb
      }
      {
        // Property: bounding_sphere
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(bounding_sphere);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GpuSubmeshData, bounding_sphere);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.get_bounding_sphere();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuSubmesh,
                                            bounding_sphere,
                                            Low::Math::Sphere);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.set_bounding_sphere(*(Low::Math::Sphere *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuSubmesh l_Handle = p_Handle.get_id();
          *((Low::Math::Sphere *)p_Data) =
              l_Handle.get_bounding_sphere();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: bounding_sphere
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(GpuSubmeshData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuSubmesh,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuSubmesh l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuSubmesh l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void GpuSubmesh::cleanup()
    {
      Low::Util::List<GpuSubmesh> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_GpuSubmesh);
      LOW_PROFILE_FREE(type_slots_GpuSubmesh);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle GpuSubmesh::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    GpuSubmesh GpuSubmesh::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      GpuSubmesh l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = GpuSubmesh::TYPE_ID;

      return l_Handle;
    }

    GpuSubmesh GpuSubmesh::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      GpuSubmesh l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = GpuSubmesh::TYPE_ID;

      return l_Handle;
    }

    bool GpuSubmesh::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == GpuSubmesh::TYPE_ID &&
             check_alive(ms_Slots, GpuSubmesh::get_capacity());
    }

    uint32_t GpuSubmesh::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    GpuSubmesh::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    GpuSubmesh GpuSubmesh::find_by_name(Low::Util::Name p_Name)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME
      // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return Low::Util::Handle::DEAD;
    }

    GpuSubmesh GpuSubmesh::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      GpuSubmesh l_Handle = make(p_Name);
      l_Handle.set_state(get_state());
      l_Handle.set_uploaded_vertex_count(get_uploaded_vertex_count());
      l_Handle.set_uploaded_index_count(get_uploaded_index_count());
      l_Handle.set_vertex_count(get_vertex_count());
      l_Handle.set_index_count(get_index_count());
      l_Handle.set_vertex_start(get_vertex_start());
      l_Handle.set_index_start(get_index_start());
      l_Handle.set_transform(get_transform());
      l_Handle.set_parent_transform(get_parent_transform());
      l_Handle.set_local_transform(get_local_transform());
      l_Handle.set_aabb(get_aabb());
      l_Handle.set_bounding_sphere(get_bounding_sphere());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    GpuSubmesh GpuSubmesh::duplicate(GpuSubmesh p_Handle,
                                     Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    GpuSubmesh::_duplicate(Low::Util::Handle p_Handle,
                           Low::Util::Name p_Name)
    {
      GpuSubmesh l_GpuSubmesh = p_Handle.get_id();
      return l_GpuSubmesh.duplicate(p_Name);
    }

    void GpuSubmesh::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void GpuSubmesh::serialize(Low::Util::Handle p_Handle,
                               Low::Util::Yaml::Node &p_Node)
    {
      GpuSubmesh l_GpuSubmesh = p_Handle.get_id();
      l_GpuSubmesh.serialize(p_Node);
    }

    Low::Util::Handle
    GpuSubmesh::deserialize(Low::Util::Yaml::Node &p_Node,
                            Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void GpuSubmesh::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 GpuSubmesh::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 GpuSubmesh::observe(Low::Util::Name p_Observable,
                            Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void GpuSubmesh::notify(Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void GpuSubmesh::_notify(Low::Util::Handle p_Observer,
                             Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
    {
      GpuSubmesh l_GpuSubmesh = p_Observer.get_id();
      l_GpuSubmesh.notify(p_Observed, p_Observable);
    }

    MeshState GpuSubmesh::get_state() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuSubmesh, state, MeshState);
    }
    void GpuSubmesh::set_state(MeshState p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuSubmesh, state, MeshState) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    uint32_t GpuSubmesh::get_uploaded_vertex_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded_vertex_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded_vertex_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuSubmesh, uploaded_vertex_count, uint32_t);
    }
    void GpuSubmesh::set_uploaded_vertex_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded_vertex_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded_vertex_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuSubmesh, uploaded_vertex_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded_vertex_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded_vertex_count

      broadcast_observable(N(uploaded_vertex_count));
    }

    uint32_t GpuSubmesh::get_uploaded_index_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded_index_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded_index_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuSubmesh, uploaded_index_count, uint32_t);
    }
    void GpuSubmesh::set_uploaded_index_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded_index_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded_index_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuSubmesh, uploaded_index_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded_index_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded_index_count

      broadcast_observable(N(uploaded_index_count));
    }

    uint32_t GpuSubmesh::get_vertex_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_vertex_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuSubmesh, vertex_count, uint32_t);
    }
    void GpuSubmesh::set_vertex_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertex_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuSubmesh, vertex_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_vertex_count

      broadcast_observable(N(vertex_count));
    }

    uint32_t GpuSubmesh::get_index_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_index_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_index_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuSubmesh, index_count, uint32_t);
    }
    void GpuSubmesh::set_index_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_index_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_index_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuSubmesh, index_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_index_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_index_count

      broadcast_observable(N(index_count));
    }

    uint32_t GpuSubmesh::get_vertex_start() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertex_start
      // LOW_CODEGEN::END::CUSTOM:GETTER_vertex_start

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuSubmesh, vertex_start, uint32_t);
    }
    void GpuSubmesh::set_vertex_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertex_start
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertex_start

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuSubmesh, vertex_start, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertex_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_vertex_start

      broadcast_observable(N(vertex_start));
    }

    uint32_t GpuSubmesh::get_index_start() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_index_start
      // LOW_CODEGEN::END::CUSTOM:GETTER_index_start

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuSubmesh, index_start, uint32_t);
    }
    void GpuSubmesh::set_index_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_index_start
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_index_start

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuSubmesh, index_start, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_index_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_index_start

      broadcast_observable(N(index_start));
    }

    Low::Math::Matrix4x4 &GpuSubmesh::get_transform() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_transform

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuSubmesh, transform, Low::Math::Matrix4x4);
    }
    void GpuSubmesh::set_transform(Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_transform

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuSubmesh, transform, Low::Math::Matrix4x4) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_transform
      // LOW_CODEGEN::END::CUSTOM:SETTER_transform

      broadcast_observable(N(transform));
    }

    Low::Math::Matrix4x4 &GpuSubmesh::get_parent_transform() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_parent_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_parent_transform

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuSubmesh, parent_transform,
                      Low::Math::Matrix4x4);
    }
    void
    GpuSubmesh::set_parent_transform(Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_parent_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_parent_transform

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuSubmesh, parent_transform, Low::Math::Matrix4x4) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_parent_transform
      // LOW_CODEGEN::END::CUSTOM:SETTER_parent_transform

      broadcast_observable(N(parent_transform));
    }

    Low::Math::Matrix4x4 &GpuSubmesh::get_local_transform() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_local_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_local_transform

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuSubmesh, local_transform,
                      Low::Math::Matrix4x4);
    }
    void
    GpuSubmesh::set_local_transform(Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_local_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_local_transform

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuSubmesh, local_transform, Low::Math::Matrix4x4) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_local_transform
      // LOW_CODEGEN::END::CUSTOM:SETTER_local_transform

      broadcast_observable(N(local_transform));
    }

    Low::Math::AABB &GpuSubmesh::get_aabb() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:GETTER_aabb

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuSubmesh, aabb, Low::Math::AABB);
    }
    void GpuSubmesh::set_aabb(Low::Math::AABB &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_aabb

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuSubmesh, aabb, Low::Math::AABB) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:SETTER_aabb

      broadcast_observable(N(aabb));
    }

    Low::Math::Sphere &GpuSubmesh::get_bounding_sphere() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:GETTER_bounding_sphere

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuSubmesh, bounding_sphere, Low::Math::Sphere);
    }
    void GpuSubmesh::set_bounding_sphere(Low::Math::Sphere &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_bounding_sphere

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuSubmesh, bounding_sphere, Low::Math::Sphere) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:SETTER_bounding_sphere

      broadcast_observable(N(bounding_sphere));
    }

    Low::Util::Name GpuSubmesh::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuSubmesh, name, Low::Util::Name);
    }
    void GpuSubmesh::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuSubmesh, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t GpuSubmesh::create_instance()
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

    void GpuSubmesh::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(GpuSubmeshData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(GpuSubmeshData, state) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GpuSubmeshData, state) *
                          (l_Capacity)],
               l_Capacity * sizeof(MeshState));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GpuSubmeshData,
                                     uploaded_vertex_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GpuSubmeshData,
                                   uploaded_vertex_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GpuSubmeshData,
                                     uploaded_index_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GpuSubmeshData,
                                   uploaded_index_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GpuSubmeshData, vertex_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GpuSubmeshData, vertex_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GpuSubmeshData, index_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GpuSubmeshData, index_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GpuSubmeshData, vertex_start) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GpuSubmeshData, vertex_start) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GpuSubmeshData, index_start) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GpuSubmeshData, index_start) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GpuSubmeshData, transform) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GpuSubmeshData, transform) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Math::Matrix4x4));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(GpuSubmeshData, parent_transform) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(GpuSubmeshData, parent_transform) *
                       (l_Capacity)],
            l_Capacity * sizeof(Low::Math::Matrix4x4));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(GpuSubmeshData, local_transform) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(GpuSubmeshData, local_transform) *
                       (l_Capacity)],
            l_Capacity * sizeof(Low::Math::Matrix4x4));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(GpuSubmeshData, aabb) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(GpuSubmeshData, aabb) * (l_Capacity)],
            l_Capacity * sizeof(Low::Math::AABB));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(GpuSubmeshData, bounding_sphere) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(GpuSubmeshData, bounding_sphere) *
                       (l_Capacity)],
            l_Capacity * sizeof(Low::Math::Sphere));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(GpuSubmeshData, name) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(GpuSubmeshData, name) * (l_Capacity)],
            l_Capacity * sizeof(Low::Util::Name));
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

      LOW_LOG_DEBUG << "Auto-increased budget for GpuSubmesh from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
