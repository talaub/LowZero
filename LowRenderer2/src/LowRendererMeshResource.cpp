#include "LowRendererMeshResource.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

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
      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, resource_mesh,
                              Util::Resource::Mesh))
          Util::Resource::Mesh();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, state,
                              MeshResourceState)) MeshResourceState();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshResource, submeshes,
                              Low::Util::List<Submesh>))
          Low::Util::List<Submesh>();
      ACCESSOR_TYPE_SOA(l_Handle, MeshResource, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_state(MeshResourceState::UNLOADED);
      l_Handle.set_submesh_count(0);
      l_Handle.set_uploaded_submesh_count(0);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void MeshResource::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const MeshResource *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
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
        // Property: resource_mesh
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resource_mesh);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResourceData, resource_mesh);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_resource_mesh();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            resource_mesh,
                                            Util::Resource::Mesh);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          *((Util::Resource::Mesh *)p_Data) =
              l_Handle.get_resource_mesh();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: resource_mesh
      }
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshResourceData, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            MeshResourceStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            state, MeshResourceState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(MeshResourceState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          *((MeshResourceState *)p_Data) = l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
      {
        // Property: submesh_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(submesh_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResourceData, submesh_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_submesh_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            submesh_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.set_submesh_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_submesh_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: submesh_count
      }
      {
        // Property: uploaded_submesh_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uploaded_submesh_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResourceData, uploaded_submesh_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_uploaded_submesh_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            uploaded_submesh_count,
                                            uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.set_uploaded_submesh_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) =
              l_Handle.get_uploaded_submesh_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: uploaded_submesh_count
      }
      {
        // Property: submeshes
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(submeshes);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResourceData, submeshes);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_submeshes();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshResource,
                                            submeshes,
                                            Low::Util::List<Submesh>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.set_submeshes(*(Low::Util::List<Submesh> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          *((Low::Util::List<Submesh> *)p_Data) =
              l_Handle.get_submeshes();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: submeshes
      }
      {
        // Property: full_meshinfo_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(full_meshinfo_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshResourceData, full_meshinfo_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.get_full_meshinfo_count();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshResource, full_meshinfo_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshResource l_Handle = p_Handle.get_id();
          l_Handle.set_full_meshinfo_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshResource l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_full_meshinfo_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: full_meshinfo_count
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
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    MeshResource MeshResource::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      MeshResource l_Handle = make(p_Name);
      l_Handle.set_path(get_path());
      l_Handle.set_state(get_state());
      l_Handle.set_submesh_count(get_submesh_count());
      l_Handle.set_uploaded_submesh_count(
          get_uploaded_submesh_count());
      l_Handle.set_submeshes(get_submeshes());
      l_Handle.set_full_meshinfo_count(get_full_meshinfo_count());

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
    }

    Util::Resource::Mesh &MeshResource::get_resource_mesh() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resource_mesh
      // LOW_CODEGEN::END::CUSTOM:GETTER_resource_mesh

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshResource, resource_mesh,
                      Util::Resource::Mesh);
    }

    MeshResourceState MeshResource::get_state() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshResource, state, MeshResourceState);
    }
    void MeshResource::set_state(MeshResourceState p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshResource, state, MeshResourceState) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state
    }

    uint32_t MeshResource::get_submesh_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_submesh_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_submesh_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshResource, submesh_count, uint32_t);
    }
    void MeshResource::set_submesh_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_submesh_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_submesh_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshResource, submesh_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_submesh_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_submesh_count
    }

    uint32_t MeshResource::get_uploaded_submesh_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded_submesh_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded_submesh_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshResource, uploaded_submesh_count, uint32_t);
    }
    void MeshResource::set_uploaded_submesh_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded_submesh_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded_submesh_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshResource, uploaded_submesh_count, uint32_t) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded_submesh_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded_submesh_count
    }

    Low::Util::List<Submesh> &MeshResource::get_submeshes() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_submeshes
      // LOW_CODEGEN::END::CUSTOM:GETTER_submeshes

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshResource, submeshes,
                      Low::Util::List<Submesh>);
    }
    void
    MeshResource::set_submeshes(Low::Util::List<Submesh> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_submeshes
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_submeshes

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshResource, submeshes, Low::Util::List<Submesh>) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_submeshes
      // LOW_CODEGEN::END::CUSTOM:SETTER_submeshes
    }

    uint32_t MeshResource::get_full_meshinfo_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_full_meshinfo_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_full_meshinfo_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshResource, full_meshinfo_count, uint32_t);
    }
    void MeshResource::set_full_meshinfo_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_full_meshinfo_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_full_meshinfo_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshResource, full_meshinfo_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_full_meshinfo_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_full_meshinfo_count
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
        memcpy(
            &l_NewBuffer[offsetof(MeshResourceData, resource_mesh) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MeshResourceData, resource_mesh) *
                       (l_Capacity)],
            l_Capacity * sizeof(Util::Resource::Mesh));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData, state) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData, state) *
                          (l_Capacity)],
               l_Capacity * sizeof(MeshResourceState));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(MeshResourceData, submesh_count) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MeshResourceData, submesh_count) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData,
                                     uploaded_submesh_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData,
                                   uploaded_submesh_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          MeshResource i_MeshResource = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(MeshResourceData, submeshes) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Low::Util::List<Submesh>))])
              Low::Util::List<Submesh>();
          *i_ValPtr =
              ACCESSOR_TYPE_SOA(i_MeshResource, MeshResource,
                                submeshes, Low::Util::List<Submesh>);
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshResourceData,
                                     full_meshinfo_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshResourceData,
                                   full_meshinfo_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
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
