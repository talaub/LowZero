#include "LowRendererSubmeshGeometry.h"

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

    const uint16_t SubmeshGeometry::TYPE_ID = 67;
    uint32_t SubmeshGeometry::ms_Capacity = 0u;
    uint32_t SubmeshGeometry::ms_PageSize = 0u;
    Low::Util::SharedMutex SubmeshGeometry::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        SubmeshGeometry::ms_PagesLock(SubmeshGeometry::ms_PagesMutex,
                                      std::defer_lock);
    Low::Util::List<SubmeshGeometry>
        SubmeshGeometry::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        SubmeshGeometry::ms_Pages;

    SubmeshGeometry::SubmeshGeometry() : Low::Util::Handle(0ull)
    {
    }
    SubmeshGeometry::SubmeshGeometry(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    SubmeshGeometry::SubmeshGeometry(SubmeshGeometry &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle SubmeshGeometry::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    SubmeshGeometry SubmeshGeometry::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      SubmeshGeometry l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = SubmeshGeometry::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, SubmeshGeometry, state,
                                 MeshState)) MeshState();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, SubmeshGeometry, vertices,
          Low::Util::List<Low::Util::Resource::Vertex>))
          Low::Util::List<Low::Util::Resource::Vertex>();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, SubmeshGeometry, indices,
                                 Low::Util::List<uint32_t>))
          Low::Util::List<uint32_t>();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, SubmeshGeometry, transform,
                                 Low::Math::Matrix4x4))
          Low::Math::Matrix4x4();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, SubmeshGeometry, parent_transform,
          Low::Math::Matrix4x4)) Low::Math::Matrix4x4();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, SubmeshGeometry, local_transform,
          Low::Math::Matrix4x4)) Low::Math::Matrix4x4();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, SubmeshGeometry, aabb,
                                 Low::Math::AABB)) Low::Math::AABB();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, SubmeshGeometry,
                                 bounding_sphere, Low::Math::Sphere))
          Low::Math::Sphere();
      ACCESSOR_TYPE_SOA(l_Handle, SubmeshGeometry, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_vertex_count(0);
      l_Handle.set_index_count(0);
      l_Handle.set_transform(LOW_MATRIX4x4_IDENTITY);
      l_Handle.set_parent_transform(LOW_MATRIX4x4_IDENTITY);
      l_Handle.set_local_transform(LOW_MATRIX4x4_IDENTITY);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void SubmeshGeometry::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());
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

    void SubmeshGeometry::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer2), N(SubmeshGeometry));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, SubmeshGeometry::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(SubmeshGeometry);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &SubmeshGeometry::is_alive;
      l_TypeInfo.destroy = &SubmeshGeometry::destroy;
      l_TypeInfo.serialize = &SubmeshGeometry::serialize;
      l_TypeInfo.deserialize = &SubmeshGeometry::deserialize;
      l_TypeInfo.find_by_index = &SubmeshGeometry::_find_by_index;
      l_TypeInfo.notify = &SubmeshGeometry::_notify;
      l_TypeInfo.find_by_name = &SubmeshGeometry::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &SubmeshGeometry::_make;
      l_TypeInfo.duplicate_default = &SubmeshGeometry::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &SubmeshGeometry::living_instances);
      l_TypeInfo.get_living_count = &SubmeshGeometry::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SubmeshGeometry::Data, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            MeshStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SubmeshGeometry,
                                            state, MeshState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(MeshState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          *((MeshState *)p_Data) = l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
      {
        // Property: vertex_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertex_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SubmeshGeometry::Data, vertex_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          l_Handle.get_vertex_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SubmeshGeometry,
                                            vertex_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          l_Handle.set_vertex_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
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
            offsetof(SubmeshGeometry::Data, index_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          l_Handle.get_index_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SubmeshGeometry,
                                            index_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          l_Handle.set_index_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          *((uint32_t *)p_Data) = l_Handle.get_index_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: index_count
      }
      {
        // Property: vertices
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertices);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SubmeshGeometry::Data, vertices);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          l_Handle.get_vertices();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SubmeshGeometry, vertices,
              Low::Util::List<Low::Util::Resource::Vertex>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          l_Handle.set_vertices(*(
              Low::Util::List<Low::Util::Resource::Vertex> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          *((Low::Util::List<Low::Util::Resource::Vertex> *)p_Data) =
              l_Handle.get_vertices();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: vertices
      }
      {
        // Property: indices
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(indices);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SubmeshGeometry::Data, indices);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          l_Handle.get_indices();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SubmeshGeometry, indices,
              Low::Util::List<uint32_t>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          l_Handle.set_indices(*(Low::Util::List<uint32_t> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          *((Low::Util::List<uint32_t> *)p_Data) =
              l_Handle.get_indices();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: indices
      }
      {
        // Property: transform
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(transform);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SubmeshGeometry::Data, transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          l_Handle.get_transform();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SubmeshGeometry,
                                            transform,
                                            Low::Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          l_Handle.set_transform(*(Low::Math::Matrix4x4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
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
            offsetof(SubmeshGeometry::Data, parent_transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          l_Handle.get_parent_transform();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SubmeshGeometry,
                                            parent_transform,
                                            Low::Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          l_Handle.set_parent_transform(
              *(Low::Math::Matrix4x4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
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
            offsetof(SubmeshGeometry::Data, local_transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          l_Handle.get_local_transform();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SubmeshGeometry,
                                            local_transform,
                                            Low::Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          l_Handle.set_local_transform(
              *(Low::Math::Matrix4x4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
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
        l_PropertyInfo.dataOffset =
            offsetof(SubmeshGeometry::Data, aabb);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          l_Handle.get_aabb();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SubmeshGeometry,
                                            aabb, Low::Math::AABB);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          l_Handle.set_aabb(*(Low::Math::AABB *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
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
            offsetof(SubmeshGeometry::Data, bounding_sphere);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          l_Handle.get_bounding_sphere();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SubmeshGeometry,
                                            bounding_sphere,
                                            Low::Math::Sphere);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          l_Handle.set_bounding_sphere(*(Low::Math::Sphere *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
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
        l_PropertyInfo.dataOffset =
            offsetof(SubmeshGeometry::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SubmeshGeometry,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SubmeshGeometry l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SubmeshGeometry> l_HandleLock(
              l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void SubmeshGeometry::cleanup()
    {
      Low::Util::List<SubmeshGeometry> l_Instances =
          ms_LivingInstances;
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

    Low::Util::Handle
    SubmeshGeometry::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    SubmeshGeometry SubmeshGeometry::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      SubmeshGeometry l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = SubmeshGeometry::TYPE_ID;

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

    SubmeshGeometry
    SubmeshGeometry::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      SubmeshGeometry l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = SubmeshGeometry::TYPE_ID;

      return l_Handle;
    }

    bool SubmeshGeometry::is_alive() const
    {
      if (m_Data.m_Type != SubmeshGeometry::TYPE_ID) {
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
      return m_Data.m_Type == SubmeshGeometry::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t SubmeshGeometry::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    SubmeshGeometry::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    SubmeshGeometry
    SubmeshGeometry::find_by_name(Low::Util::Name p_Name)
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

    SubmeshGeometry
    SubmeshGeometry::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      SubmeshGeometry l_Handle = make(p_Name);
      l_Handle.set_state(get_state());
      l_Handle.set_vertex_count(get_vertex_count());
      l_Handle.set_index_count(get_index_count());
      l_Handle.set_vertices(get_vertices());
      l_Handle.set_indices(get_indices());
      l_Handle.set_transform(get_transform());
      l_Handle.set_parent_transform(get_parent_transform());
      l_Handle.set_local_transform(get_local_transform());
      l_Handle.set_aabb(get_aabb());
      l_Handle.set_bounding_sphere(get_bounding_sphere());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    SubmeshGeometry
    SubmeshGeometry::duplicate(SubmeshGeometry p_Handle,
                               Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    SubmeshGeometry::_duplicate(Low::Util::Handle p_Handle,
                                Low::Util::Name p_Name)
    {
      SubmeshGeometry l_SubmeshGeometry = p_Handle.get_id();
      return l_SubmeshGeometry.duplicate(p_Name);
    }

    void
    SubmeshGeometry::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void SubmeshGeometry::serialize(Low::Util::Handle p_Handle,
                                    Low::Util::Yaml::Node &p_Node)
    {
      SubmeshGeometry l_SubmeshGeometry = p_Handle.get_id();
      l_SubmeshGeometry.serialize(p_Node);
    }

    Low::Util::Handle
    SubmeshGeometry::deserialize(Low::Util::Yaml::Node &p_Node,
                                 Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void SubmeshGeometry::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 SubmeshGeometry::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 SubmeshGeometry::observe(Low::Util::Name p_Observable,
                                 Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void SubmeshGeometry::notify(Low::Util::Handle p_Observed,
                                 Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void SubmeshGeometry::_notify(Low::Util::Handle p_Observer,
                                  Low::Util::Handle p_Observed,
                                  Low::Util::Name p_Observable)
    {
      SubmeshGeometry l_SubmeshGeometry = p_Observer.get_id();
      l_SubmeshGeometry.notify(p_Observed, p_Observable);
    }

    MeshState SubmeshGeometry::get_state() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      return TYPE_SOA(SubmeshGeometry, state, MeshState);
    }
    void SubmeshGeometry::set_state(MeshState p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      TYPE_SOA(SubmeshGeometry, state, MeshState) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    uint32_t SubmeshGeometry::get_vertex_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_vertex_count

      return TYPE_SOA(SubmeshGeometry, vertex_count, uint32_t);
    }
    void SubmeshGeometry::set_vertex_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertex_count

      // Set new value
      TYPE_SOA(SubmeshGeometry, vertex_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_vertex_count

      broadcast_observable(N(vertex_count));
    }

    uint32_t SubmeshGeometry::get_index_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_index_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_index_count

      return TYPE_SOA(SubmeshGeometry, index_count, uint32_t);
    }
    void SubmeshGeometry::set_index_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_index_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_index_count

      // Set new value
      TYPE_SOA(SubmeshGeometry, index_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_index_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_index_count

      broadcast_observable(N(index_count));
    }

    Low::Util::List<Low::Util::Resource::Vertex> &
    SubmeshGeometry::get_vertices() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertices
      // LOW_CODEGEN::END::CUSTOM:GETTER_vertices

      return TYPE_SOA(SubmeshGeometry, vertices,
                      Low::Util::List<Low::Util::Resource::Vertex>);
    }
    void SubmeshGeometry::set_vertices(
        Low::Util::List<Low::Util::Resource::Vertex> &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertices
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertices

      // Set new value
      TYPE_SOA(SubmeshGeometry, vertices,
               Low::Util::List<Low::Util::Resource::Vertex>) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertices
      // LOW_CODEGEN::END::CUSTOM:SETTER_vertices

      broadcast_observable(N(vertices));
    }

    Low::Util::List<uint32_t> &SubmeshGeometry::get_indices() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_indices
      // LOW_CODEGEN::END::CUSTOM:GETTER_indices

      return TYPE_SOA(SubmeshGeometry, indices,
                      Low::Util::List<uint32_t>);
    }
    void
    SubmeshGeometry::set_indices(Low::Util::List<uint32_t> &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_indices
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_indices

      // Set new value
      TYPE_SOA(SubmeshGeometry, indices, Low::Util::List<uint32_t>) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_indices
      // LOW_CODEGEN::END::CUSTOM:SETTER_indices

      broadcast_observable(N(indices));
    }

    Low::Math::Matrix4x4 &SubmeshGeometry::get_transform() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_transform

      return TYPE_SOA(SubmeshGeometry, transform,
                      Low::Math::Matrix4x4);
    }
    void SubmeshGeometry::set_transform(Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_transform

      // Set new value
      TYPE_SOA(SubmeshGeometry, transform, Low::Math::Matrix4x4) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_transform
      // LOW_CODEGEN::END::CUSTOM:SETTER_transform

      broadcast_observable(N(transform));
    }

    Low::Math::Matrix4x4 &
    SubmeshGeometry::get_parent_transform() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_parent_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_parent_transform

      return TYPE_SOA(SubmeshGeometry, parent_transform,
                      Low::Math::Matrix4x4);
    }
    void SubmeshGeometry::set_parent_transform(
        Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_parent_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_parent_transform

      // Set new value
      TYPE_SOA(SubmeshGeometry, parent_transform,
               Low::Math::Matrix4x4) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_parent_transform
      // LOW_CODEGEN::END::CUSTOM:SETTER_parent_transform

      broadcast_observable(N(parent_transform));
    }

    Low::Math::Matrix4x4 &SubmeshGeometry::get_local_transform() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_local_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_local_transform

      return TYPE_SOA(SubmeshGeometry, local_transform,
                      Low::Math::Matrix4x4);
    }
    void SubmeshGeometry::set_local_transform(
        Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_local_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_local_transform

      // Set new value
      TYPE_SOA(SubmeshGeometry, local_transform,
               Low::Math::Matrix4x4) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_local_transform
      // LOW_CODEGEN::END::CUSTOM:SETTER_local_transform

      broadcast_observable(N(local_transform));
    }

    Low::Math::AABB &SubmeshGeometry::get_aabb() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:GETTER_aabb

      return TYPE_SOA(SubmeshGeometry, aabb, Low::Math::AABB);
    }
    void SubmeshGeometry::set_aabb(Low::Math::AABB &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_aabb

      // Set new value
      TYPE_SOA(SubmeshGeometry, aabb, Low::Math::AABB) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:SETTER_aabb

      broadcast_observable(N(aabb));
    }

    Low::Math::Sphere &SubmeshGeometry::get_bounding_sphere() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:GETTER_bounding_sphere

      return TYPE_SOA(SubmeshGeometry, bounding_sphere,
                      Low::Math::Sphere);
    }
    void
    SubmeshGeometry::set_bounding_sphere(Low::Math::Sphere &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_bounding_sphere

      // Set new value
      TYPE_SOA(SubmeshGeometry, bounding_sphere, Low::Math::Sphere) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:SETTER_bounding_sphere

      broadcast_observable(N(bounding_sphere));
    }

    Low::Util::Name SubmeshGeometry::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(SubmeshGeometry, name, Low::Util::Name);
    }
    void SubmeshGeometry::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SubmeshGeometry> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(SubmeshGeometry, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t SubmeshGeometry::create_instance(
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

    u32 SubmeshGeometry::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for SubmeshGeometry.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, SubmeshGeometry::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool SubmeshGeometry::get_page_for_index(const u32 p_Index,
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
