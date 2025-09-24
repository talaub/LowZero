#include "LowRendererGpuSubmesh.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
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
    uint32_t GpuSubmesh::ms_PageSize = 0u;
    Low::Util::SharedMutex GpuSubmesh::ms_LivingMutex;
    Low::Util::SharedMutex GpuSubmesh::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        GpuSubmesh::ms_PagesLock(GpuSubmesh::ms_PagesMutex,
                                 std::defer_lock);
    Low::Util::List<GpuSubmesh> GpuSubmesh::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        GpuSubmesh::ms_Pages;

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
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      GpuSubmesh l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = GpuSubmesh::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, GpuSubmesh, state,
                                 MeshState)) MeshState();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, GpuSubmesh, transform,
                                 Low::Math::Matrix4x4))
          Low::Math::Matrix4x4();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, GpuSubmesh, parent_transform,
          Low::Math::Matrix4x4)) Low::Math::Matrix4x4();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, GpuSubmesh, local_transform,
          Low::Math::Matrix4x4)) Low::Math::Matrix4x4();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, GpuSubmesh, aabb,
                                 Low::Math::AABB)) Low::Math::AABB();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, GpuSubmesh,
                                 bounding_sphere, Low::Math::Sphere))
          Low::Math::Sphere();
      ACCESSOR_TYPE_SOA(l_Handle, GpuSubmesh, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      {
        Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        ms_LivingInstances.push_back(l_Handle);
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void GpuSubmesh::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      _LOW_ASSERT(
          get_page_for_index(get_index(), l_PageIndex, l_SlotIndex));
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
          l_Page->mutex);
      l_Page->slots[l_SlotIndex].m_Occupied = false;
      l_Page->slots[l_SlotIndex].m_Generation++;

      ms_PagesLock.lock();
      Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
          ms_LivingMutex);
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end();) {
        if (it->get_id() == get_id()) {
          it = ms_LivingInstances.erase(it);
        } else {
          it++;
        }
      }
      ms_PagesLock.unlock();
      l_LivingLock.unlock();
    }

    void GpuSubmesh::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(GpuSubmesh));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, GpuSubmesh::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

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
        l_PropertyInfo.dataOffset = offsetof(GpuSubmesh::Data, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            MeshStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
            offsetof(GpuSubmesh::Data, uploaded_vertex_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
            offsetof(GpuSubmesh::Data, uploaded_index_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
            offsetof(GpuSubmesh::Data, vertex_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
            offsetof(GpuSubmesh::Data, index_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
            offsetof(GpuSubmesh::Data, vertex_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
            offsetof(GpuSubmesh::Data, index_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
            offsetof(GpuSubmesh::Data, transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
            offsetof(GpuSubmesh::Data, parent_transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
            offsetof(GpuSubmesh::Data, local_transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset = offsetof(GpuSubmesh::Data, aabb);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
            offsetof(GpuSubmesh::Data, bounding_sphere);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset = offsetof(GpuSubmesh::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuSubmesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuSubmesh> l_HandleLock(l_Handle);
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

    Low::Util::Handle GpuSubmesh::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    GpuSubmesh GpuSubmesh::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      GpuSubmesh l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = GpuSubmesh::TYPE_ID;

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
      if (m_Data.m_Type != GpuSubmesh::TYPE_ID) {
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
      return m_Data.m_Type == GpuSubmesh::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
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

      Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
          ms_LivingMutex);
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
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      return TYPE_SOA(GpuSubmesh, state, MeshState);
    }
    void GpuSubmesh::set_state(MeshState p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      TYPE_SOA(GpuSubmesh, state, MeshState) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    uint32_t GpuSubmesh::get_uploaded_vertex_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded_vertex_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded_vertex_count

      return TYPE_SOA(GpuSubmesh, uploaded_vertex_count, uint32_t);
    }
    void GpuSubmesh::set_uploaded_vertex_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded_vertex_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded_vertex_count

      // Set new value
      TYPE_SOA(GpuSubmesh, uploaded_vertex_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded_vertex_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded_vertex_count

      broadcast_observable(N(uploaded_vertex_count));
    }

    uint32_t GpuSubmesh::get_uploaded_index_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded_index_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded_index_count

      return TYPE_SOA(GpuSubmesh, uploaded_index_count, uint32_t);
    }
    void GpuSubmesh::set_uploaded_index_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded_index_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded_index_count

      // Set new value
      TYPE_SOA(GpuSubmesh, uploaded_index_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded_index_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded_index_count

      broadcast_observable(N(uploaded_index_count));
    }

    uint32_t GpuSubmesh::get_vertex_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_vertex_count

      return TYPE_SOA(GpuSubmesh, vertex_count, uint32_t);
    }
    void GpuSubmesh::set_vertex_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertex_count

      // Set new value
      TYPE_SOA(GpuSubmesh, vertex_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_vertex_count

      broadcast_observable(N(vertex_count));
    }

    uint32_t GpuSubmesh::get_index_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_index_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_index_count

      return TYPE_SOA(GpuSubmesh, index_count, uint32_t);
    }
    void GpuSubmesh::set_index_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_index_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_index_count

      // Set new value
      TYPE_SOA(GpuSubmesh, index_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_index_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_index_count

      broadcast_observable(N(index_count));
    }

    uint32_t GpuSubmesh::get_vertex_start() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertex_start
      // LOW_CODEGEN::END::CUSTOM:GETTER_vertex_start

      return TYPE_SOA(GpuSubmesh, vertex_start, uint32_t);
    }
    void GpuSubmesh::set_vertex_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertex_start
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertex_start

      // Set new value
      TYPE_SOA(GpuSubmesh, vertex_start, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertex_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_vertex_start

      broadcast_observable(N(vertex_start));
    }

    uint32_t GpuSubmesh::get_index_start() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_index_start
      // LOW_CODEGEN::END::CUSTOM:GETTER_index_start

      return TYPE_SOA(GpuSubmesh, index_start, uint32_t);
    }
    void GpuSubmesh::set_index_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_index_start
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_index_start

      // Set new value
      TYPE_SOA(GpuSubmesh, index_start, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_index_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_index_start

      broadcast_observable(N(index_start));
    }

    Low::Math::Matrix4x4 &GpuSubmesh::get_transform() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_transform

      return TYPE_SOA(GpuSubmesh, transform, Low::Math::Matrix4x4);
    }
    void GpuSubmesh::set_transform(Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_transform

      // Set new value
      TYPE_SOA(GpuSubmesh, transform, Low::Math::Matrix4x4) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_transform
      // LOW_CODEGEN::END::CUSTOM:SETTER_transform

      broadcast_observable(N(transform));
    }

    Low::Math::Matrix4x4 &GpuSubmesh::get_parent_transform() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_parent_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_parent_transform

      return TYPE_SOA(GpuSubmesh, parent_transform,
                      Low::Math::Matrix4x4);
    }
    void
    GpuSubmesh::set_parent_transform(Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_parent_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_parent_transform

      // Set new value
      TYPE_SOA(GpuSubmesh, parent_transform, Low::Math::Matrix4x4) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_parent_transform
      // LOW_CODEGEN::END::CUSTOM:SETTER_parent_transform

      broadcast_observable(N(parent_transform));
    }

    Low::Math::Matrix4x4 &GpuSubmesh::get_local_transform() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_local_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_local_transform

      return TYPE_SOA(GpuSubmesh, local_transform,
                      Low::Math::Matrix4x4);
    }
    void
    GpuSubmesh::set_local_transform(Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_local_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_local_transform

      // Set new value
      TYPE_SOA(GpuSubmesh, local_transform, Low::Math::Matrix4x4) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_local_transform
      // LOW_CODEGEN::END::CUSTOM:SETTER_local_transform

      broadcast_observable(N(local_transform));
    }

    Low::Math::AABB &GpuSubmesh::get_aabb() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:GETTER_aabb

      return TYPE_SOA(GpuSubmesh, aabb, Low::Math::AABB);
    }
    void GpuSubmesh::set_aabb(Low::Math::AABB &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_aabb

      // Set new value
      TYPE_SOA(GpuSubmesh, aabb, Low::Math::AABB) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:SETTER_aabb

      broadcast_observable(N(aabb));
    }

    Low::Math::Sphere &GpuSubmesh::get_bounding_sphere() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:GETTER_bounding_sphere

      return TYPE_SOA(GpuSubmesh, bounding_sphere, Low::Math::Sphere);
    }
    void GpuSubmesh::set_bounding_sphere(Low::Math::Sphere &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_bounding_sphere

      // Set new value
      TYPE_SOA(GpuSubmesh, bounding_sphere, Low::Math::Sphere) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:SETTER_bounding_sphere

      broadcast_observable(N(bounding_sphere));
    }

    Low::Util::Name GpuSubmesh::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(GpuSubmesh, name, Low::Util::Name);
    }
    void GpuSubmesh::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuSubmesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(GpuSubmesh, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t GpuSubmesh::create_instance(
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
          if (!ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied) {
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

    u32 GpuSubmesh::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for GpuSubmesh.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, GpuSubmesh::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool GpuSubmesh::get_page_for_index(const u32 p_Index,
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

  } // namespace Renderer
} // namespace Low
