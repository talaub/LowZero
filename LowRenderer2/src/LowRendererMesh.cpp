#include "LowRendererMesh.h"

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

    const uint16_t Mesh::TYPE_ID = 46;
    uint32_t Mesh::ms_Capacity = 0u;
    uint8_t *Mesh::ms_Buffer = 0;
    std::shared_mutex Mesh::ms_BufferMutex;
    Low::Util::Instances::Slot *Mesh::ms_Slots = 0;
    Low::Util::List<Mesh> Mesh::ms_LivingInstances =
        Low::Util::List<Mesh>();

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
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      Mesh l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Mesh::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Mesh, resource,
                              Low::Renderer::MeshResource))
          Low::Renderer::MeshResource();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Mesh, state, MeshState))
          MeshState();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Mesh, geometry,
                              Low::Renderer::MeshGeometry))
          Low::Renderer::MeshGeometry();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Mesh, gpu,
                              Low::Renderer::GpuMesh))
          Low::Renderer::GpuMesh();
      ACCESSOR_TYPE_SOA(l_Handle, Mesh, name, Low::Util::Name) =
          Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_state(MeshState::UNLOADED);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Mesh::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

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

      broadcast_observable(OBSERVABLE_DESTROY);

      WRITE_LOCK(l_Lock);
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
    }

    void Mesh::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowRenderer2), N(Mesh));

      initialize_buffer(&ms_Buffer, MeshData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_Mesh);
      LOW_PROFILE_ALLOC(type_slots_Mesh);

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
        l_PropertyInfo.dataOffset = offsetof(MeshData, resource);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::MeshResource::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(MeshData, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            MeshStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(MeshData, geometry);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::MeshGeometry::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(MeshData, gpu);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::GpuMesh::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
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
          *((Low::Renderer::GpuMesh *)p_Data) = l_Handle.get_gpu();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: gpu
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
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
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Mesh::cleanup()
    {
      Low::Util::List<Mesh> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Mesh);
      LOW_PROFILE_FREE(type_slots_Mesh);
      LOCK_UNLOCK(l_Lock);
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
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = Mesh::TYPE_ID;

      return l_Handle;
    }

    bool Mesh::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == Mesh::TYPE_ID &&
             check_alive(ms_Slots, Mesh::get_capacity());
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
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
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

    Low::Renderer::MeshResource Mesh::get_resource() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resource
      // LOW_CODEGEN::END::CUSTOM:GETTER_resource

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Mesh, resource, Low::Renderer::MeshResource);
    }
    void Mesh::set_resource(Low::Renderer::MeshResource p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_resource
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_resource

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Mesh, resource, Low::Renderer::MeshResource) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_resource
      // LOW_CODEGEN::END::CUSTOM:SETTER_resource

      broadcast_observable(N(resource));
    }

    MeshState Mesh::get_state() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Mesh, state, MeshState);
    }
    void Mesh::set_state(MeshState p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Mesh, state, MeshState) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    Low::Renderer::MeshGeometry Mesh::get_geometry() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_geometry
      // LOW_CODEGEN::END::CUSTOM:GETTER_geometry

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Mesh, geometry, Low::Renderer::MeshGeometry);
    }
    void Mesh::set_geometry(Low::Renderer::MeshGeometry p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_geometry
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_geometry

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Mesh, geometry, Low::Renderer::MeshGeometry) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_geometry
      // LOW_CODEGEN::END::CUSTOM:SETTER_geometry

      broadcast_observable(N(geometry));
    }

    Low::Renderer::GpuMesh Mesh::get_gpu() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gpu
      // LOW_CODEGEN::END::CUSTOM:GETTER_gpu

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Mesh, gpu, Low::Renderer::GpuMesh);
    }
    void Mesh::set_gpu(Low::Renderer::GpuMesh p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gpu
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gpu

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Mesh, gpu, Low::Renderer::GpuMesh) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gpu
      // LOW_CODEGEN::END::CUSTOM:SETTER_gpu

      broadcast_observable(N(gpu));
    }

    Low::Util::Name Mesh::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Mesh, name, Low::Util::Name);
    }
    void Mesh::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Mesh, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    Mesh Mesh::make_from_resource_config(MeshResourceConfig &p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_from_resource_config
      Mesh l_Mesh = Mesh::make(p_Config.name);
      l_Mesh.set_resource(MeshResource::make_from_config(p_Config));
      l_Mesh.set_state(MeshState::UNLOADED);

      return l_Mesh;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_from_resource_config
    }

    uint32_t Mesh::create_instance()
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

    void Mesh::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(MeshData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(
            &l_NewBuffer[offsetof(MeshData, resource) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MeshData, resource) * (l_Capacity)],
            l_Capacity * sizeof(Low::Renderer::MeshResource));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshData, state) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshData, state) * (l_Capacity)],
               l_Capacity * sizeof(MeshState));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(MeshData, geometry) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MeshData, geometry) * (l_Capacity)],
            l_Capacity * sizeof(Low::Renderer::MeshGeometry));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshData, gpu) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshData, gpu) * (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::GpuMesh));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for Mesh from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
