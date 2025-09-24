#include "LowRendererMesh.h"

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
#include "LowRendererResourceManager.h"
#include "LowUtilHashing.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Mesh::TYPE_ID = 46;
    uint32_t Mesh::ms_Capacity = 0u;
    uint32_t Mesh::ms_PageSize = 0u;
    Low::Util::SharedMutex Mesh::ms_LivingMutex;
    Low::Util::SharedMutex Mesh::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        Mesh::ms_PagesLock(Mesh::ms_PagesMutex, std::defer_lock);
    Low::Util::List<Mesh> Mesh::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *> Mesh::ms_Pages;

    Mesh::Mesh() : Low::Util::Handle(0ull)
    {
    }
    Mesh::Mesh(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Mesh::Mesh(Mesh &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Mesh::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Mesh Mesh::make(Low::Util::Name p_Name)
    {
      return make(p_Name, 0ull);
    }

    Mesh Mesh::make(Low::Util::Name p_Name,
                    Low::Util::UniqueId p_UniqueId)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      Mesh l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = Mesh::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Mesh, resource,
                                 Low::Renderer::MeshResource))
          Low::Renderer::MeshResource();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Mesh, state, MeshState))
          MeshState();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Mesh, geometry,
                                 Low::Renderer::MeshGeometry))
          Low::Renderer::MeshGeometry();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Mesh, gpu,
                                 Low::Renderer::GpuMesh))
          Low::Renderer::GpuMesh();
      ACCESSOR_TYPE_SOA(l_Handle, Mesh, unloadable, bool) = false;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Mesh, references,
                                 Low::Util::Set<u64>))
          Low::Util::Set<u64>();
      ACCESSOR_TYPE_SOA(l_Handle, Mesh, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      {
        Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        ms_LivingInstances.push_back(l_Handle);
      }

      if (p_UniqueId > 0ull) {
        l_Handle.set_unique_id(p_UniqueId);
      } else {
        l_Handle.set_unique_id(
            Low::Util::generate_unique_id(l_Handle.get_id()));
      }
      Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                    l_Handle.get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_state(MeshState::UNLOADED);
      l_Handle.set_unloadable(true);

      ResourceManager::register_asset(l_Handle.get_unique_id(),
                                      l_Handle);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Mesh::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<Mesh> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // TODO: Unload if loaded
        if (get_gpu().is_alive()) {
          get_gpu().destroy();
        }
        if (get_geometry().is_alive()) {
          get_geometry().destroy();
        }
        if (get_resource().is_alive()) {
          get_resource().destroy();
        }
        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      Low::Util::remove_unique_id(get_unique_id());

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

    void Mesh::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowRenderer2), N(Mesh));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, Mesh::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Mesh);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Mesh::is_alive;
      l_TypeInfo.destroy = &Mesh::destroy;
      l_TypeInfo.serialize = &Mesh::serialize;
      l_TypeInfo.deserialize = &Mesh::deserialize;
      l_TypeInfo.find_by_index = &Mesh::_find_by_index;
      l_TypeInfo.notify = &Mesh::_notify;
      l_TypeInfo.find_by_name = &Mesh::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Mesh::_make;
      l_TypeInfo.duplicate_default = &Mesh::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Mesh::living_instances);
      l_TypeInfo.get_living_count = &Mesh::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: resource
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resource);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Mesh::Data, resource);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::MeshResource::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.get_resource();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Mesh, resource, Low::Renderer::MeshResource);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_resource(
              *(Low::Renderer::MeshResource *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((Low::Renderer::MeshResource *)p_Data) =
              l_Handle.get_resource();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: resource
      }
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Mesh::Data, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            MeshStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh, state,
                                            MeshState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(MeshState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((MeshState *)p_Data) = l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
      {
        // Property: geometry
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(geometry);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Mesh::Data, geometry);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::MeshGeometry::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.get_geometry();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Mesh, geometry, Low::Renderer::MeshGeometry);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_geometry(
              *(Low::Renderer::MeshGeometry *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((Low::Renderer::MeshGeometry *)p_Data) =
              l_Handle.get_geometry();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: geometry
      }
      {
        // Property: gpu
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(gpu);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Mesh::Data, gpu);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::GpuMesh::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.get_gpu();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh, gpu,
                                            Low::Renderer::GpuMesh);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_gpu(*(Low::Renderer::GpuMesh *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((Low::Renderer::GpuMesh *)p_Data) = l_Handle.get_gpu();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: gpu
      }
      {
        // Property: unloadable
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(unloadable);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Mesh::Data, unloadable);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.is_unloadable();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh,
                                            unloadable, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_unloadable(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((bool *)p_Data) = l_Handle.is_unloadable();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: unloadable
      }
      {
        // Property: submesh_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(submesh_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Mesh::Data, submesh_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.get_submesh_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh,
                                            submesh_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_submesh_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((uint32_t *)p_Data) = l_Handle.get_submesh_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: submesh_count
      }
      {
        // Property: references
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(references);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Mesh::Data, references);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: references
      }
      {
        // Property: unique_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(unique_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Mesh::Data, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh, unique_id,
                                            Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((Low::Util::UniqueId *)p_Data) = l_Handle.get_unique_id();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: unique_id
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Mesh::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: make_from_resource_config
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make_from_resource_config);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = Mesh::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Config);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make_from_resource_config
      }
      {
        // Function: get_editor_image
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_editor_image);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = EditorImage::TYPE_ID;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_editor_image
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Mesh::cleanup()
    {
      Low::Util::List<Mesh> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle Mesh::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    Mesh Mesh::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Mesh l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = Mesh::TYPE_ID;

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

    Mesh Mesh::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      Mesh l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = Mesh::TYPE_ID;

      return l_Handle;
    }

    bool Mesh::is_alive() const
    {
      if (m_Data.m_Type != Mesh::TYPE_ID) {
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
      return m_Data.m_Type == Mesh::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t Mesh::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle Mesh::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    Mesh Mesh::find_by_name(Low::Util::Name p_Name)
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

    Mesh Mesh::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      Mesh l_Handle = make(p_Name);
      if (get_resource().is_alive()) {
        l_Handle.set_resource(get_resource());
      }
      l_Handle.set_state(get_state());
      if (get_geometry().is_alive()) {
        l_Handle.set_geometry(get_geometry());
      }
      if (get_gpu().is_alive()) {
        l_Handle.set_gpu(get_gpu());
      }
      l_Handle.set_unloadable(is_unloadable());
      l_Handle.set_submesh_count(get_submesh_count());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    Mesh Mesh::duplicate(Mesh p_Handle, Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle Mesh::_duplicate(Low::Util::Handle p_Handle,
                                       Low::Util::Name p_Name)
    {
      Mesh l_Mesh = p_Handle.get_id();
      return l_Mesh.duplicate(p_Name);
    }

    void Mesh::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Mesh::serialize(Low::Util::Handle p_Handle,
                         Low::Util::Yaml::Node &p_Node)
    {
      Mesh l_Mesh = p_Handle.get_id();
      l_Mesh.serialize(p_Node);
    }

    Low::Util::Handle Mesh::deserialize(Low::Util::Yaml::Node &p_Node,
                                        Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void
    Mesh::broadcast_observable(Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 Mesh::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 Mesh::observe(Low::Util::Name p_Observable,
                      Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void Mesh::notify(Low::Util::Handle p_Observed,
                      Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void Mesh::_notify(Low::Util::Handle p_Observer,
                       Low::Util::Handle p_Observed,
                       Low::Util::Name p_Observable)
    {
      Mesh l_Mesh = p_Observer.get_id();
      l_Mesh.notify(p_Observed, p_Observable);
    }

    void Mesh::reference(const u64 p_Id)
    {
      _LOW_ASSERT(is_alive());

      Low::Util::HandleLock<Mesh> l_HandleLock(get_id());
      const u32 l_OldReferences =
          (TYPE_SOA(Mesh, references, Low::Util::Set<u64>)).size();

      (TYPE_SOA(Mesh, references, Low::Util::Set<u64>)).insert(p_Id);

      const u32 l_References =
          (TYPE_SOA(Mesh, references, Low::Util::Set<u64>)).size();

      if (l_OldReferences != l_References) {
        // LOW_CODEGEN:BEGIN:CUSTOM:NEW_REFERENCE
        if (l_References > 0 && get_state() == MeshState::UNLOADED) {
          ResourceManager::load_mesh(get_id());
        }
        // LOW_CODEGEN::END::CUSTOM:NEW_REFERENCE
      }
    }

    void Mesh::dereference(const u64 p_Id)
    {
      _LOW_ASSERT(is_alive());

      Low::Util::HandleLock<Mesh> l_HandleLock(get_id());
      const u32 l_OldReferences =
          (TYPE_SOA(Mesh, references, Low::Util::Set<u64>)).size();

      (TYPE_SOA(Mesh, references, Low::Util::Set<u64>)).erase(p_Id);

      const u32 l_References =
          (TYPE_SOA(Mesh, references, Low::Util::Set<u64>)).size();

      if (l_OldReferences != l_References) {
        // LOW_CODEGEN:BEGIN:CUSTOM:REFERENCE_REMOVED
        // LOW_CODEGEN::END::CUSTOM:REFERENCE_REMOVED
      }
    }

    u32 Mesh::references() const
    {
      return get_references().size();
    }

    Low::Renderer::MeshResource Mesh::get_resource() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resource
      // LOW_CODEGEN::END::CUSTOM:GETTER_resource

      return TYPE_SOA(Mesh, resource, Low::Renderer::MeshResource);
    }
    void Mesh::set_resource(Low::Renderer::MeshResource p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_resource
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_resource

      // Set new value
      TYPE_SOA(Mesh, resource, Low::Renderer::MeshResource) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_resource
      // LOW_CODEGEN::END::CUSTOM:SETTER_resource

      broadcast_observable(N(resource));
    }

    MeshState Mesh::get_state() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      return TYPE_SOA(Mesh, state, MeshState);
    }
    void Mesh::set_state(MeshState p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      TYPE_SOA(Mesh, state, MeshState) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    Low::Renderer::MeshGeometry Mesh::get_geometry() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_geometry
      // LOW_CODEGEN::END::CUSTOM:GETTER_geometry

      return TYPE_SOA(Mesh, geometry, Low::Renderer::MeshGeometry);
    }
    void Mesh::set_geometry(Low::Renderer::MeshGeometry p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_geometry
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_geometry

      // Set new value
      TYPE_SOA(Mesh, geometry, Low::Renderer::MeshGeometry) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_geometry
      LOCK_HANDLE(p_Value);
      if (p_Value.is_alive()) {
        set_submesh_count(p_Value.get_submesh_count());
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_geometry

      broadcast_observable(N(geometry));
    }

    Low::Renderer::GpuMesh Mesh::get_gpu() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gpu
      // LOW_CODEGEN::END::CUSTOM:GETTER_gpu

      return TYPE_SOA(Mesh, gpu, Low::Renderer::GpuMesh);
    }
    void Mesh::set_gpu(Low::Renderer::GpuMesh p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gpu
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gpu

      // Set new value
      TYPE_SOA(Mesh, gpu, Low::Renderer::GpuMesh) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gpu
      // LOW_CODEGEN::END::CUSTOM:SETTER_gpu

      broadcast_observable(N(gpu));
    }

    bool Mesh::is_unloadable() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unloadable
      // LOW_CODEGEN::END::CUSTOM:GETTER_unloadable

      return TYPE_SOA(Mesh, unloadable, bool);
    }
    void Mesh::toggle_unloadable()
    {
      set_unloadable(!is_unloadable());
    }

    void Mesh::set_unloadable(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unloadable
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unloadable

      // Set new value
      TYPE_SOA(Mesh, unloadable, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unloadable
      // LOW_CODEGEN::END::CUSTOM:SETTER_unloadable

      broadcast_observable(N(unloadable));
    }

    uint32_t Mesh::get_submesh_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_submesh_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_submesh_count

      return TYPE_SOA(Mesh, submesh_count, uint32_t);
    }
    void Mesh::set_submesh_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_submesh_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_submesh_count

      // Set new value
      TYPE_SOA(Mesh, submesh_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_submesh_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_submesh_count

      broadcast_observable(N(submesh_count));
    }

    Low::Util::Set<u64> &Mesh::get_references() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_references
      // LOW_CODEGEN::END::CUSTOM:GETTER_references

      return TYPE_SOA(Mesh, references, Low::Util::Set<u64>);
    }

    Low::Util::UniqueId Mesh::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(Mesh, unique_id, Low::Util::UniqueId);
    }
    void Mesh::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(Mesh, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

      broadcast_observable(N(unique_id));
    }

    Low::Util::Name Mesh::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Mesh, name, Low::Util::Name);
    }
    void Mesh::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Mesh, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    Mesh Mesh::make_from_resource_config(MeshResourceConfig &p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_from_resource_config
      MeshResource l_Resource =
          MeshResource::make_from_config(p_Config);
      Mesh l_Mesh =
          Mesh::make(p_Config.name, l_Resource.get_mesh_id());
      l_Mesh.set_resource(l_Resource);
      l_Mesh.set_state(MeshState::UNLOADED);

      l_Mesh.set_submesh_count(p_Config.submeshCount);

      ResourceManager::register_asset(l_Mesh.get_unique_id(), l_Mesh);

      return l_Mesh;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_from_resource_config
    }

    EditorImage Mesh::get_editor_image()
    {
      Low::Util::HandleLock<Mesh> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_editor_image
      if (get_resource().is_alive()) {
        Util::String l_ImageName = "mesh_";
        l_ImageName +=
            Util::hash_to_string(get_resource().get_mesh_id());
        return EditorImage::find_by_name(
            LOW_NAME(l_ImageName.c_str()));
      }
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_editor_image
    }

    uint32_t Mesh::create_instance(
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

    u32 Mesh::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for Mesh.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, Mesh::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool Mesh::get_page_for_index(const u32 p_Index, u32 &p_PageIndex,
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
