#include "LowRendererMeshResource.h"

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

    const uint16_t MeshResource::TYPE_ID = 47;
    uint32_t MeshResource::ms_Capacity = 0u;
    uint8_t *MeshResource::ms_Buffer = 0;
    std::shared_mutex MeshResource::ms_BufferMutex;
    Low::Util::Instances::Slot *MeshResource::ms_Slots = 0;
    Low::Util::List<MeshResource> MeshResource::ms_LivingInstances =
        Low::Util::List<MeshResource>();

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
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      MeshResource l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = MeshResource::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, path,
                              Util::String)) Util::String();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, mesh_path,
                              Util::String)) Util::String();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, sidecar_path,
                              Util::String)) Util::String();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, source_file,
                              Util::String)) Util::String();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, mesh_id,
                              uint64_t)) uint64_t();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, asset_hash,
                              uint64_t)) uint64_t();
      ACCESSOR_TYPE_SOA(l_Handle, MeshResource, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void MeshResource::destroy()
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

    void MeshResource::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(MeshResource));

      initialize_buffer(&ms_Buffer, MeshResourceData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_MeshResource);
      LOW_PROFILE_ALLOC(type_slots_MeshResource);

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
        l_PropertyInfo.dataOffset = offsetof(MeshResourceData, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          *((Util::String *)p_Data) = l_Handle.get_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: path
      }
      {
        // Property: mesh_path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(mesh_path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResourceData, mesh_path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_mesh_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            mesh_path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          *((Util::String *)p_Data) = l_Handle.get_mesh_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: mesh_path
      }
      {
        // Property: sidecar_path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(sidecar_path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResourceData, sidecar_path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_sidecar_path();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshResource, sidecar_path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          *((Util::String *)p_Data) = l_Handle.get_sidecar_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: sidecar_path
      }
      {
        // Property: source_file
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(source_file);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResourceData, source_file);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_source_file();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshResource, source_file, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          *((Util::String *)p_Data) = l_Handle.get_source_file();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: source_file
      }
      {
        // Property: mesh_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(mesh_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResourceData, mesh_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_mesh_id();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            mesh_id, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_mesh_id();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: mesh_id
      }
      {
        // Property: asset_hash
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(asset_hash);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResourceData, asset_hash);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_asset_hash();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            asset_hash, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_asset_hash();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: asset_hash
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshResourceData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
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
        // Function: make_from_config
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make_from_config);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = MeshResource::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Config);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make_from_config
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void MeshResource::cleanup()
    {
      Low::Util::List<MeshResource> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_MeshResource);
      LOW_PROFILE_FREE(type_slots_MeshResource);
      LOCK_UNLOCK(l_Lock);
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
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = MeshResource::TYPE_ID;

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
      READ_LOCK(l_Lock);
      return m_Data.m_Type == MeshResource::TYPE_ID &&
             check_alive(ms_Slots, MeshResource::get_capacity());
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
      l_Handle.set_mesh_path(get_mesh_path());
      l_Handle.set_sidecar_path(get_sidecar_path());
      l_Handle.set_source_file(get_source_file());
      l_Handle.set_mesh_id(get_mesh_id());
      l_Handle.set_asset_hash(get_asset_hash());

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
      return 0;
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

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      READ_LOCK(l_ReadLock);
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

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshResource, path, Util::String) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_path

      broadcast_observable(N(path));
    }

    Util::String &MeshResource::get_mesh_path() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mesh_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_mesh_path

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshResource, mesh_path, Util::String);
    }
    void MeshResource::set_mesh_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_mesh_path(l_Val);
    }

    void MeshResource::set_mesh_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mesh_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_mesh_path

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshResource, mesh_path, Util::String) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mesh_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_mesh_path

      broadcast_observable(N(mesh_path));
    }

    Util::String &MeshResource::get_sidecar_path() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_sidecar_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_sidecar_path

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshResource, sidecar_path, Util::String);
    }
    void MeshResource::set_sidecar_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_sidecar_path(l_Val);
    }

    void MeshResource::set_sidecar_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_sidecar_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_sidecar_path

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshResource, sidecar_path, Util::String) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_sidecar_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_sidecar_path

      broadcast_observable(N(sidecar_path));
    }

    Util::String &MeshResource::get_source_file() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_source_file
      // LOW_CODEGEN::END::CUSTOM:GETTER_source_file

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshResource, source_file, Util::String);
    }
    void MeshResource::set_source_file(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_source_file(l_Val);
    }

    void MeshResource::set_source_file(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_source_file
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_source_file

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshResource, source_file, Util::String) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_source_file
      // LOW_CODEGEN::END::CUSTOM:SETTER_source_file

      broadcast_observable(N(source_file));
    }

    uint64_t &MeshResource::get_mesh_id() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mesh_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_mesh_id

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshResource, mesh_id, uint64_t);
    }
    void MeshResource::set_mesh_id(uint64_t &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mesh_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_mesh_id

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshResource, mesh_id, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mesh_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_mesh_id

      broadcast_observable(N(mesh_id));
    }

    uint64_t &MeshResource::get_asset_hash() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_asset_hash
      // LOW_CODEGEN::END::CUSTOM:GETTER_asset_hash

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshResource, asset_hash, uint64_t);
    }
    void MeshResource::set_asset_hash(uint64_t &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_asset_hash
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_asset_hash

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshResource, asset_hash, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_asset_hash
      // LOW_CODEGEN::END::CUSTOM:SETTER_asset_hash

      broadcast_observable(N(asset_hash));
    }

    Low::Util::Name MeshResource::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshResource, name, Low::Util::Name);
    }
    void MeshResource::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshResource, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

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
      MeshResource l_MeshResource =
          MeshResource::make(LOW_NAME(l_FileName.c_str()));
      l_MeshResource.set_path(p_Path);

      return l_MeshResource;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    MeshResource
    MeshResource::make_from_config(MeshResourceConfig &p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_from_config
      MeshResource l_MeshResource = MeshResource::make(p_Config.name);
      l_MeshResource.set_path(p_Config.path);
      l_MeshResource.set_mesh_id(p_Config.meshId);
      l_MeshResource.set_asset_hash(p_Config.assetHash);
      l_MeshResource.set_source_file(p_Config.sourceFile);
      l_MeshResource.set_sidecar_path(p_Config.sidecarPath);
      l_MeshResource.set_mesh_path(p_Config.meshPath);

      return l_MeshResource;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_from_config
    }

    uint32_t MeshResource::create_instance()
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

    void MeshResource::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer =
          (uint8_t *)malloc((l_Capacity + l_CapacityIncrease) *
                            sizeof(MeshResourceData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, path) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, path) *
                          (l_Capacity)],
               l_Capacity * sizeof(Util::String));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, mesh_path) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, mesh_path) *
                          (l_Capacity)],
               l_Capacity * sizeof(Util::String));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, sidecar_path) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, sidecar_path) *
                          (l_Capacity)],
               l_Capacity * sizeof(Util::String));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, source_file) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, source_file) *
                          (l_Capacity)],
               l_Capacity * sizeof(Util::String));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, mesh_id) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, mesh_id) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, asset_hash) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, asset_hash) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, name) *
                          (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for MeshResource from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
