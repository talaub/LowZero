#include "LowRendererGpuMesh.h"

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

    const uint16_t GpuMesh::TYPE_ID = 68;
    uint32_t GpuMesh::ms_Capacity = 0u;
    uint8_t *GpuMesh::ms_Buffer = 0;
    std::shared_mutex GpuMesh::ms_BufferMutex;
    Low::Util::Instances::Slot *GpuMesh::ms_Slots = 0;
    Low::Util::List<GpuMesh> GpuMesh::ms_LivingInstances =
        Low::Util::List<GpuMesh>();

    GpuMesh::GpuMesh() : Low::Util::Handle(0ull)
    {
    }
    GpuMesh::GpuMesh(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    GpuMesh::GpuMesh(GpuMesh &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle GpuMesh::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    GpuMesh GpuMesh::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      GpuMesh l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = GpuMesh::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, GpuMesh, submeshes,
                              Low::Util::List<GpuSubmesh>))
          Low::Util::List<GpuSubmesh>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GpuMesh, aabb,
                              Low::Math::AABB)) Low::Math::AABB();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GpuMesh, bounding_sphere,
                              Low::Math::Sphere)) Low::Math::Sphere();
      ACCESSOR_TYPE_SOA(l_Handle, GpuMesh, name, Low::Util::Name) =
          Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void GpuMesh::destroy()
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

    void GpuMesh::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(GpuMesh));

      initialize_buffer(&ms_Buffer, GpuMeshData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_GpuMesh);
      LOW_PROFILE_ALLOC(type_slots_GpuMesh);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(GpuMesh);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &GpuMesh::is_alive;
      l_TypeInfo.destroy = &GpuMesh::destroy;
      l_TypeInfo.serialize = &GpuMesh::serialize;
      l_TypeInfo.deserialize = &GpuMesh::deserialize;
      l_TypeInfo.find_by_index = &GpuMesh::_find_by_index;
      l_TypeInfo.notify = &GpuMesh::_notify;
      l_TypeInfo.find_by_name = &GpuMesh::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &GpuMesh::_make;
      l_TypeInfo.duplicate_default = &GpuMesh::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &GpuMesh::living_instances);
      l_TypeInfo.get_living_count = &GpuMesh::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: uploaded_submesh_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uploaded_submesh_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GpuMeshData, uploaded_submesh_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMesh l_Handle = p_Handle.get_id();
          l_Handle.get_uploaded_submesh_count();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GpuMesh, uploaded_submesh_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuMesh l_Handle = p_Handle.get_id();
          l_Handle.set_uploaded_submesh_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuMesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) =
              l_Handle.get_uploaded_submesh_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: uploaded_submesh_count
      }
      {
        // Property: submesh_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(submesh_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GpuMeshData, submesh_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMesh l_Handle = p_Handle.get_id();
          l_Handle.get_submesh_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuMesh,
                                            submesh_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuMesh l_Handle = p_Handle.get_id();
          l_Handle.set_submesh_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuMesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_submesh_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: submesh_count
      }
      {
        // Property: submeshes
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(submeshes);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(GpuMeshData, submeshes);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMesh l_Handle = p_Handle.get_id();
          l_Handle.get_submeshes();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GpuMesh, submeshes,
              Low::Util::List<GpuSubmesh>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuMesh l_Handle = p_Handle.get_id();
          l_Handle.set_submeshes(
              *(Low::Util::List<GpuSubmesh> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuMesh l_Handle = p_Handle.get_id();
          *((Low::Util::List<GpuSubmesh> *)p_Data) =
              l_Handle.get_submeshes();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: submeshes
      }
      {
        // Property: aabb
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(aabb);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(GpuMeshData, aabb);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMesh l_Handle = p_Handle.get_id();
          l_Handle.get_aabb();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuMesh, aabb,
                                            Low::Math::AABB);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuMesh l_Handle = p_Handle.get_id();
          l_Handle.set_aabb(*(Low::Math::AABB *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuMesh l_Handle = p_Handle.get_id();
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
            offsetof(GpuMeshData, bounding_sphere);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMesh l_Handle = p_Handle.get_id();
          l_Handle.get_bounding_sphere();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GpuMesh, bounding_sphere, Low::Math::Sphere);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuMesh l_Handle = p_Handle.get_id();
          l_Handle.set_bounding_sphere(*(Low::Math::Sphere *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuMesh l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(GpuMeshData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMesh l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuMesh, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuMesh l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuMesh l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void GpuMesh::cleanup()
    {
      Low::Util::List<GpuMesh> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_GpuMesh);
      LOW_PROFILE_FREE(type_slots_GpuMesh);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle GpuMesh::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    GpuMesh GpuMesh::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      GpuMesh l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = GpuMesh::TYPE_ID;

      return l_Handle;
    }

    GpuMesh GpuMesh::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      GpuMesh l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = GpuMesh::TYPE_ID;

      return l_Handle;
    }

    bool GpuMesh::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == GpuMesh::TYPE_ID &&
             check_alive(ms_Slots, GpuMesh::get_capacity());
    }

    uint32_t GpuMesh::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle GpuMesh::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    GpuMesh GpuMesh::find_by_name(Low::Util::Name p_Name)
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

    GpuMesh GpuMesh::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      GpuMesh l_Handle = make(p_Name);
      l_Handle.set_uploaded_submesh_count(
          get_uploaded_submesh_count());
      l_Handle.set_submesh_count(get_submesh_count());
      l_Handle.set_submeshes(get_submeshes());
      l_Handle.set_aabb(get_aabb());
      l_Handle.set_bounding_sphere(get_bounding_sphere());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    GpuMesh GpuMesh::duplicate(GpuMesh p_Handle,
                               Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle GpuMesh::_duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name)
    {
      GpuMesh l_GpuMesh = p_Handle.get_id();
      return l_GpuMesh.duplicate(p_Name);
    }

    void GpuMesh::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void GpuMesh::serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node)
    {
      GpuMesh l_GpuMesh = p_Handle.get_id();
      l_GpuMesh.serialize(p_Node);
    }

    Low::Util::Handle
    GpuMesh::deserialize(Low::Util::Yaml::Node &p_Node,
                         Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void
    GpuMesh::broadcast_observable(Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 GpuMesh::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 GpuMesh::observe(Low::Util::Name p_Observable,
                         Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void GpuMesh::notify(Low::Util::Handle p_Observed,
                         Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void GpuMesh::_notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
    {
      GpuMesh l_GpuMesh = p_Observer.get_id();
      l_GpuMesh.notify(p_Observed, p_Observable);
    }

    uint32_t GpuMesh::get_uploaded_submesh_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded_submesh_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded_submesh_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuMesh, uploaded_submesh_count, uint32_t);
    }
    void GpuMesh::set_uploaded_submesh_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded_submesh_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded_submesh_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuMesh, uploaded_submesh_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded_submesh_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded_submesh_count

      broadcast_observable(N(uploaded_submesh_count));
    }

    uint32_t GpuMesh::get_submesh_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_submesh_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_submesh_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuMesh, submesh_count, uint32_t);
    }
    void GpuMesh::set_submesh_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_submesh_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_submesh_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuMesh, submesh_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_submesh_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_submesh_count

      broadcast_observable(N(submesh_count));
    }

    Low::Util::List<GpuSubmesh> &GpuMesh::get_submeshes() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_submeshes
      // LOW_CODEGEN::END::CUSTOM:GETTER_submeshes

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuMesh, submeshes,
                      Low::Util::List<GpuSubmesh>);
    }
    void GpuMesh::set_submeshes(Low::Util::List<GpuSubmesh> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_submeshes
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_submeshes

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuMesh, submeshes, Low::Util::List<GpuSubmesh>) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_submeshes
      // LOW_CODEGEN::END::CUSTOM:SETTER_submeshes

      broadcast_observable(N(submeshes));
    }

    Low::Math::AABB &GpuMesh::get_aabb() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:GETTER_aabb

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuMesh, aabb, Low::Math::AABB);
    }
    void GpuMesh::set_aabb(Low::Math::AABB &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_aabb

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuMesh, aabb, Low::Math::AABB) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:SETTER_aabb

      broadcast_observable(N(aabb));
    }

    Low::Math::Sphere &GpuMesh::get_bounding_sphere() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:GETTER_bounding_sphere

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuMesh, bounding_sphere, Low::Math::Sphere);
    }
    void GpuMesh::set_bounding_sphere(Low::Math::Sphere &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_bounding_sphere

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuMesh, bounding_sphere, Low::Math::Sphere) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:SETTER_bounding_sphere

      broadcast_observable(N(bounding_sphere));
    }

    Low::Util::Name GpuMesh::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(GpuMesh, name, Low::Util::Name);
    }
    void GpuMesh::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(GpuMesh, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t GpuMesh::create_instance()
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

    void GpuMesh::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(GpuMeshData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(
            &l_NewBuffer[offsetof(GpuMeshData,
                                  uploaded_submesh_count) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(GpuMeshData, uploaded_submesh_count) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GpuMeshData, submesh_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GpuMeshData, submesh_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          GpuMesh i_GpuMesh = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(GpuMeshData, submeshes) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Low::Util::List<GpuSubmesh>))])
              Low::Util::List<GpuSubmesh>();
          *i_ValPtr = ACCESSOR_TYPE_SOA(i_GpuMesh, GpuMesh, submeshes,
                                        Low::Util::List<GpuSubmesh>);
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(GpuMeshData, aabb) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GpuMeshData, aabb) * (l_Capacity)],
               l_Capacity * sizeof(Low::Math::AABB));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GpuMeshData, bounding_sphere) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GpuMeshData, bounding_sphere) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Math::Sphere));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GpuMeshData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GpuMeshData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for GpuMesh from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
