#include "LowCoreMeshResource.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowUtilResource.h"
#include "LowUtilJobManager.h"
#include "LowRenderer.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

#define MESH_COUNT 50
    Util::List<bool> g_MeshSlots;
    Util::List<Util::Resource::Mesh> g_Meshes;

    struct MeshLoadSchedule
    {
      uint32_t meshIndex;
      Util::Future<void> future;
      MeshResource meshResource;

      MeshLoadSchedule(uint64_t p_Id, Util::Future<void> p_Future)
          : meshIndex(p_Id), future(std::move(p_Future))
      {
      }
    };

    Util::List<MeshLoadSchedule> g_MeshLoadSchedules;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t MeshResource::TYPE_ID = 21;
    uint32_t MeshResource::ms_Capacity = 0u;
    uint32_t MeshResource::ms_PageSize = 0u;
    Low::Util::SharedMutex MeshResource::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        MeshResource::ms_PagesLock(MeshResource::ms_PagesMutex,
                                   std::defer_lock);
    Low::Util::List<MeshResource> MeshResource::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        MeshResource::ms_Pages;

    MeshResource::MeshResource() : Low::Util::Handle(0ull)
    {
    }
    MeshResource::MeshResource(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    MeshResource::MeshResource(MeshResource &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle MeshResource::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    MeshResource MeshResource::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      MeshResource l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = MeshResource::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<MeshResource> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, MeshResource, path,
                                 Util::String)) Util::String();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, MeshResource, submeshes,
                                 Util::List<Submesh>))
          Util::List<Submesh>();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, MeshResource, skeleton,
                                 Renderer::Skeleton))
          Renderer::Skeleton();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, MeshResource, state,
                                 ResourceState)) ResourceState();
      ACCESSOR_TYPE_SOA(l_Handle, MeshResource, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      l_Handle.set_reference_count(0);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void MeshResource::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<MeshResource> l_Lock(get_id());
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

    void MeshResource::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      g_Meshes.resize(MESH_COUNT);
      g_MeshSlots.resize(MESH_COUNT);
      for (uint32_t i = 0; i < MESH_COUNT; ++i) {
        g_MeshSlots[i] = false;
      }
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowCore),
                                                    N(MeshResource));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, MeshResource::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(MeshResource);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &MeshResource::is_alive;
      l_TypeInfo.destroy = &MeshResource::destroy;
      l_TypeInfo.serialize = &MeshResource::serialize;
      l_TypeInfo.deserialize = &MeshResource::deserialize;
      l_TypeInfo.find_by_index = &MeshResource::_find_by_index;
      l_TypeInfo.notify = &MeshResource::_notify;
      l_TypeInfo.find_by_name = &MeshResource::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &MeshResource::_make;
      l_TypeInfo.duplicate_default = &MeshResource::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &MeshResource::living_instances);
      l_TypeInfo.get_living_count = &MeshResource::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResource::Data, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshResource> l_HandleLock(l_Handle);
          l_Handle.get_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshResource> l_HandleLock(l_Handle);
          *((Util::String *)p_Data) = l_Handle.get_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: path
      }
      {
        // Property: submeshes
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(submeshes);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResource::Data, submeshes);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshResource> l_HandleLock(l_Handle);
          l_Handle.get_submeshes();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshResource, submeshes, Util::List<Submesh>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshResource> l_HandleLock(l_Handle);
          *((Util::List<Submesh> *)p_Data) = l_Handle.get_submeshes();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: submeshes
      }
      {
        // Property: reference_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(reference_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResource::Data, reference_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
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
        // End property: reference_count
      }
      {
        // Property: skeleton
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(skeleton);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResource::Data, skeleton);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Renderer::Skeleton::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshResource> l_HandleLock(l_Handle);
          l_Handle.get_skeleton();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshResource, skeleton, Renderer::Skeleton);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshResource> l_HandleLock(l_Handle);
          *((Renderer::Skeleton *)p_Data) = l_Handle.get_skeleton();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: skeleton
      }
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResource::Data, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshResource> l_HandleLock(l_Handle);
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            state, ResourceState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(ResourceState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshResource> l_HandleLock(l_Handle);
          *((ResourceState *)p_Data) = l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResource::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshResource> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshResource> l_HandleLock(l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: make
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = MeshResource::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Path);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::STRING;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: is_loaded
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(is_loaded);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: is_loaded
      }
      {
        // Function: load
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(load);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: load
      }
      {
        // Function: _load
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(_load);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_MeshIndex);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UINT32;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: _load
      }
      {
        // Function: _internal_load
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(_internal_load);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Mesh);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: _internal_load
      }
      {
        // Function: unload
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(unload);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: unload
      }
      {
        // Function: _unload
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(_unload);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: _unload
      }
      {
        // Function: update
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(update);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: update
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void MeshResource::cleanup()
    {
      Low::Util::List<MeshResource> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle MeshResource::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    MeshResource MeshResource::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      MeshResource l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = MeshResource::TYPE_ID;

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

    MeshResource MeshResource::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      MeshResource l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = MeshResource::TYPE_ID;

      return l_Handle;
    }

    bool MeshResource::is_alive() const
    {
      if (m_Data.m_Type != MeshResource::TYPE_ID) {
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
      return m_Data.m_Type == MeshResource::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t MeshResource::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    MeshResource::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    MeshResource MeshResource::find_by_name(Low::Util::Name p_Name)
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

    MeshResource MeshResource::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      MeshResource l_Handle = make(p_Name);
      l_Handle.set_path(get_path());
      l_Handle.set_reference_count(get_reference_count());
      if (get_skeleton().is_alive()) {
        l_Handle.set_skeleton(get_skeleton());
      }
      l_Handle.set_state(get_state());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    MeshResource MeshResource::duplicate(MeshResource p_Handle,
                                         Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    MeshResource::_duplicate(Low::Util::Handle p_Handle,
                             Low::Util::Name p_Name)
    {
      MeshResource l_MeshResource = p_Handle.get_id();
      return l_MeshResource.duplicate(p_Name);
    }

    void MeshResource::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      p_Node = get_path().c_str();
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void MeshResource::serialize(Low::Util::Handle p_Handle,
                                 Low::Util::Yaml::Node &p_Node)
    {
      MeshResource l_MeshResource = p_Handle.get_id();
      l_MeshResource.serialize(p_Node);
    }

    Low::Util::Handle
    MeshResource::deserialize(Low::Util::Yaml::Node &p_Node,
                              Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      MeshResource l_Resource =
          MeshResource::make(LOW_YAML_AS_STRING(p_Node));

      return l_Resource;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void MeshResource::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 MeshResource::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 MeshResource::observe(Low::Util::Name p_Observable,
                              Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void MeshResource::notify(Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void MeshResource::_notify(Low::Util::Handle p_Observer,
                               Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
    {
      MeshResource l_MeshResource = p_Observer.get_id();
      l_MeshResource.notify(p_Observed, p_Observable);
    }

    Util::String &MeshResource::get_path() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path

      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      return TYPE_SOA(MeshResource, path, Util::String);
    }
    void MeshResource::set_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_path(l_Val);
    }

    void MeshResource::set_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      TYPE_SOA(MeshResource, path, Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path

      // LOW_CODEGEN::END::CUSTOM:SETTER_path

      broadcast_observable(N(path));
    }

    Util::List<Submesh> &MeshResource::get_submeshes() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_submeshes

      // LOW_CODEGEN::END::CUSTOM:GETTER_submeshes

      return TYPE_SOA(MeshResource, submeshes, Util::List<Submesh>);
    }

    uint32_t MeshResource::get_reference_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:GETTER_reference_count

      return TYPE_SOA(MeshResource, reference_count, uint32_t);
    }
    void MeshResource::set_reference_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_reference_count

      // Set new value
      TYPE_SOA(MeshResource, reference_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:SETTER_reference_count

      broadcast_observable(N(reference_count));
    }

    Renderer::Skeleton MeshResource::get_skeleton() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_skeleton

      // LOW_CODEGEN::END::CUSTOM:GETTER_skeleton

      return TYPE_SOA(MeshResource, skeleton, Renderer::Skeleton);
    }
    void MeshResource::set_skeleton(Renderer::Skeleton p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_skeleton

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_skeleton

      // Set new value
      TYPE_SOA(MeshResource, skeleton, Renderer::Skeleton) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_skeleton

      // LOW_CODEGEN::END::CUSTOM:SETTER_skeleton

      broadcast_observable(N(skeleton));
    }

    ResourceState MeshResource::get_state() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state

      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      return TYPE_SOA(MeshResource, state, ResourceState);
    }
    void MeshResource::set_state(ResourceState p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      TYPE_SOA(MeshResource, state, ResourceState) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state

      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    Low::Util::Name MeshResource::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(MeshResource, name, Low::Util::Name);
    }
    void MeshResource::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(MeshResource, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    MeshResource MeshResource::make(Util::String &p_Path)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_path() == p_Path) {
          return *it;
        }
      }

      Util::String l_FileName =
          p_Path.substr(p_Path.find_last_of("/\\") + 1);
      MeshResource l_Mesh =
          MeshResource::make(LOW_NAME(l_FileName.c_str()));
      l_Mesh.set_path(p_Path);

      return l_Mesh;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    bool MeshResource::is_loaded()
    {
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_loaded

      return get_state() == ResourceState::LOADED;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_loaded
    }

    void MeshResource::load()
    {
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_load

      LOW_ASSERT(is_alive(), "Mesh resource was not alive on load");

      set_reference_count(get_reference_count() + 1);

      LOW_ASSERT(get_reference_count() > 0,
                 "Increased MeshResource reference count, but its "
                 "not over 0. "
                 "Something went wrong.");

      if (get_state() != ResourceState::UNLOADED) {
        return;
      }

      set_state(ResourceState::STREAMING);

      Util::String l_P = get_path();
      uint32_t l_MeshIndex = 0;
      bool l_FoundIndex = false;

      Util::String l_FullPath =
          Util::get_project().dataPath + "\\resources\\meshes\\";
      l_FullPath += get_path().c_str();

#if 1
      do {
        for (uint32_t i = 0u; i < MESH_COUNT; ++i) {
          if (!g_MeshSlots[i]) {
            l_MeshIndex = i;
            l_FoundIndex = true;
            g_MeshSlots[i] = true;
            break;
          }
        }
      } while (!l_FoundIndex);

      MeshLoadSchedule &l_LoadSchedule =
          g_MeshLoadSchedules.emplace_back(
              l_MeshIndex,
              Util::JobManager::default_pool().enqueue(
                  [l_FullPath, l_MeshIndex]() {
                    for (auto it = g_MeshLoadSchedules.begin();
                         it != g_MeshLoadSchedules.end(); ++it) {
                      if (it->meshIndex == l_MeshIndex) {
                        Util::Resource::load_mesh(
                            l_FullPath, g_Meshes[l_MeshIndex]);
                        break;
                      }
                    }
                  }));

      l_LoadSchedule.meshResource = *this;
#else
      Util::Resource::Mesh l_Mesh;
      Util::Resource::load_mesh(l_FullPath, l_Mesh);

      _internal_load(l_Mesh);
#endif

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_load
    }

    void MeshResource::_load(uint32_t p_MeshIndex)
    {
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION__load
      Util::Resource::Mesh &l_Mesh = g_Meshes[p_MeshIndex];

      _internal_load(l_Mesh);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION__load
    }

    void
    MeshResource::_internal_load(Low::Util::Resource::Mesh &p_Mesh)
    {
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION__internal_load
      Util::List<Submesh> &l_Submeshes = get_submeshes();

      for (uint32_t i = 0u; i < p_Mesh.submeshes.size(); ++i) {
        for (uint32_t j = 0u;
             j < p_Mesh.submeshes[i].meshInfos.size(); ++j) {
          Submesh i_Submesh;
          i_Submesh.name = p_Mesh.submeshes[i].name;
          i_Submesh.transformation = p_Mesh.submeshes[i].transform;
          i_Submesh.mesh = Renderer::upload_mesh(
              N(Submesh), p_Mesh.submeshes[i].meshInfos[j]);

          l_Submeshes.push_back(i_Submesh);
        }
      }

      if (!p_Mesh.animations.empty()) {
        set_skeleton(Renderer::upload_skeleton(N(Skeleton), p_Mesh));
      }

      set_state(ResourceState::LOADED);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION__internal_load
    }

    void MeshResource::unload()
    {
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_unload

      set_reference_count(get_reference_count() - 1);

      LOW_ASSERT(get_reference_count() >= 0,
                 "MeshResource reference count < 0. Something "
                 "went wrong.");

      if (get_reference_count() <= 0) {
        _unload();
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_unload
    }

    void MeshResource::_unload()
    {
      Low::Util::HandleLock<MeshResource> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION__unload

      if (!is_loaded()) {
        return;
      }
      for (auto it = get_submeshes().begin();
           it != get_submeshes().end(); ++it) {
        Renderer::unload_mesh(it->mesh);
      }

      get_submeshes().clear();

      if (get_skeleton().is_alive()) {
        Renderer::unload_skeleton(get_skeleton());
      }
      set_state(ResourceState::UNLOADED);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION__unload
    }

    void MeshResource::update()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update

      LOW_PROFILE_CPU("Core", "Update MeshResource");
      for (auto it = g_MeshLoadSchedules.begin();
           it != g_MeshLoadSchedules.end();) {
        if (it->future.wait_for(std::chrono::seconds(0)) ==
            std::future_status::ready) {
          it->meshResource._load(it->meshIndex);
          g_MeshSlots[it->meshIndex] = false;

          it = g_MeshLoadSchedules.erase(it);
        } else {
          ++it;
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_update
    }

    uint32_t MeshResource::create_instance(
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

    u32 MeshResource::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for MeshResource.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, MeshResource::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool MeshResource::get_page_for_index(const u32 p_Index,
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

  } // namespace Core
} // namespace Low
