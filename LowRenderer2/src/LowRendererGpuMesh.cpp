#include "LowRendererGpuMesh.h"

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

    const uint16_t GpuMesh::TYPE_ID = 68;
    uint32_t GpuMesh::ms_Capacity = 0u;
    uint32_t GpuMesh::ms_PageSize = 0u;
    Low::Util::SharedMutex GpuMesh::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        GpuMesh::ms_PagesLock(GpuMesh::ms_PagesMutex,
                              std::defer_lock);
    Low::Util::List<GpuMesh> GpuMesh::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *> GpuMesh::ms_Pages;

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
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      GpuMesh l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = GpuMesh::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<GpuMesh> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, GpuMesh, submeshes,
                                 Low::Util::List<GpuSubmesh>))
          Low::Util::List<GpuSubmesh>();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, GpuMesh, aabb,
                                 Low::Math::AABB)) Low::Math::AABB();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, GpuMesh, bounding_sphere,
                                 Low::Math::Sphere))
          Low::Math::Sphere();
      ACCESSOR_TYPE_SOA(l_Handle, GpuMesh, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void GpuMesh::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<GpuMesh> l_Lock(get_id());
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

    void GpuMesh::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(GpuMesh));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, GpuMesh::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

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
            offsetof(GpuMesh::Data, uploaded_submesh_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuMesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuMesh> l_HandleLock(l_Handle);
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
            offsetof(GpuMesh::Data, submesh_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuMesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuMesh> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset =
            offsetof(GpuMesh::Data, submeshes);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuMesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuMesh> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset = offsetof(GpuMesh::Data, aabb);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuMesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuMesh> l_HandleLock(l_Handle);
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
            offsetof(GpuMesh::Data, bounding_sphere);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuMesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuMesh> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset = offsetof(GpuMesh::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<GpuMesh> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<GpuMesh> l_HandleLock(l_Handle);
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

    Low::Util::Handle GpuMesh::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    GpuMesh GpuMesh::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      GpuMesh l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = GpuMesh::TYPE_ID;

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
      if (m_Data.m_Type != GpuMesh::TYPE_ID) {
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
      return m_Data.m_Type == GpuMesh::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
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
      Low::Util::HandleLock<GpuMesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded_submesh_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded_submesh_count

      return TYPE_SOA(GpuMesh, uploaded_submesh_count, uint32_t);
    }
    void GpuMesh::set_uploaded_submesh_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuMesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded_submesh_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded_submesh_count

      // Set new value
      TYPE_SOA(GpuMesh, uploaded_submesh_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded_submesh_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded_submesh_count

      broadcast_observable(N(uploaded_submesh_count));
    }

    uint32_t GpuMesh::get_submesh_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuMesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_submesh_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_submesh_count

      return TYPE_SOA(GpuMesh, submesh_count, uint32_t);
    }
    void GpuMesh::set_submesh_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuMesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_submesh_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_submesh_count

      // Set new value
      TYPE_SOA(GpuMesh, submesh_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_submesh_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_submesh_count

      broadcast_observable(N(submesh_count));
    }

    Low::Util::List<GpuSubmesh> &GpuMesh::get_submeshes() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuMesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_submeshes
      // LOW_CODEGEN::END::CUSTOM:GETTER_submeshes

      return TYPE_SOA(GpuMesh, submeshes,
                      Low::Util::List<GpuSubmesh>);
    }
    void GpuMesh::set_submeshes(Low::Util::List<GpuSubmesh> &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuMesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_submeshes
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_submeshes

      // Set new value
      TYPE_SOA(GpuMesh, submeshes, Low::Util::List<GpuSubmesh>) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_submeshes
      // LOW_CODEGEN::END::CUSTOM:SETTER_submeshes

      broadcast_observable(N(submeshes));
    }

    Low::Math::AABB &GpuMesh::get_aabb() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuMesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:GETTER_aabb

      return TYPE_SOA(GpuMesh, aabb, Low::Math::AABB);
    }
    void GpuMesh::set_aabb(Low::Math::AABB &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuMesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_aabb

      // Set new value
      TYPE_SOA(GpuMesh, aabb, Low::Math::AABB) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_aabb
      // LOW_CODEGEN::END::CUSTOM:SETTER_aabb

      broadcast_observable(N(aabb));
    }

    Low::Math::Sphere &GpuMesh::get_bounding_sphere() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuMesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:GETTER_bounding_sphere

      return TYPE_SOA(GpuMesh, bounding_sphere, Low::Math::Sphere);
    }
    void GpuMesh::set_bounding_sphere(Low::Math::Sphere &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuMesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_bounding_sphere

      // Set new value
      TYPE_SOA(GpuMesh, bounding_sphere, Low::Math::Sphere) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_bounding_sphere
      // LOW_CODEGEN::END::CUSTOM:SETTER_bounding_sphere

      broadcast_observable(N(bounding_sphere));
    }

    Low::Util::Name GpuMesh::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuMesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(GpuMesh, name, Low::Util::Name);
    }
    void GpuMesh::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<GpuMesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(GpuMesh, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t GpuMesh::create_instance(
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

    u32 GpuMesh::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for GpuMesh.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, GpuMesh::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool GpuMesh::get_page_for_index(const u32 p_Index,
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
