#include "LowRendererMeshInfo.h"

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

    const uint16_t MeshInfo::TYPE_ID = 51;
    uint32_t MeshInfo::ms_Capacity = 0u;
    uint8_t *MeshInfo::ms_Buffer = 0;
    std::shared_mutex MeshInfo::ms_BufferMutex;
    Low::Util::Instances::Slot *MeshInfo::ms_Slots = 0;
    Low::Util::List<MeshInfo> MeshInfo::ms_LivingInstances =
        Low::Util::List<MeshInfo>();

    MeshInfo::MeshInfo() : Low::Util::Handle(0ull)
    {
    }
    MeshInfo::MeshInfo(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    MeshInfo::MeshInfo(MeshInfo &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle MeshInfo::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    MeshInfo MeshInfo::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      MeshInfo l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = MeshInfo::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshInfo, state,
                              MeshResourceState)) MeshResourceState();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshInfo, submesh, Submesh))
          Submesh();
      ACCESSOR_TYPE_SOA(l_Handle, MeshInfo, name, Low::Util::Name) =
          Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void MeshInfo::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const MeshInfo *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void MeshInfo::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(MeshInfo));

      initialize_buffer(&ms_Buffer, MeshInfoData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_MeshInfo);
      LOW_PROFILE_ALLOC(type_slots_MeshInfo);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(MeshInfo);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &MeshInfo::is_alive;
      l_TypeInfo.destroy = &MeshInfo::destroy;
      l_TypeInfo.serialize = &MeshInfo::serialize;
      l_TypeInfo.deserialize = &MeshInfo::deserialize;
      l_TypeInfo.find_by_index = &MeshInfo::_find_by_index;
      l_TypeInfo.find_by_name = &MeshInfo::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &MeshInfo::_make;
      l_TypeInfo.duplicate_default = &MeshInfo::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &MeshInfo::living_instances);
      l_TypeInfo.get_living_count = &MeshInfo::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshInfoData, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            MeshResourceStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshInfo, state,
                                            MeshResourceState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(MeshResourceState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInfo l_Handle = p_Handle.get_id();
          *((MeshResourceState *)p_Data) = l_Handle.get_state();
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
            offsetof(MeshInfoData, vertex_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.get_vertex_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshInfo,
                                            vertex_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.set_vertex_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInfo l_Handle = p_Handle.get_id();
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
            offsetof(MeshInfoData, index_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.get_index_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshInfo,
                                            index_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.set_index_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInfo l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_index_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: index_count
      }
      {
        // Property: uploaded_vertex_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uploaded_vertex_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshInfoData, uploaded_vertex_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.get_uploaded_vertex_count();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshInfo, uploaded_vertex_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.set_uploaded_vertex_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInfo l_Handle = p_Handle.get_id();
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
            offsetof(MeshInfoData, uploaded_index_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.get_uploaded_index_count();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshInfo, uploaded_index_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.set_uploaded_index_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInfo l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_uploaded_index_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: uploaded_index_count
      }
      {
        // Property: vertex_start
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertex_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshInfoData, vertex_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.get_vertex_start();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshInfo,
                                            vertex_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.set_vertex_start(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInfo l_Handle = p_Handle.get_id();
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
            offsetof(MeshInfoData, index_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.get_index_start();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshInfo,
                                            index_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.set_index_start(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInfo l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_index_start();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: index_start
      }
      {
        // Property: submesh
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(submesh);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshInfoData, submesh);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Submesh::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.get_submesh();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshInfo,
                                            submesh, Submesh);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInfo l_Handle = p_Handle.get_id();
          *((Submesh *)p_Data) = l_Handle.get_submesh();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: submesh
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshInfoData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshInfo, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshInfo l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInfo l_Handle = p_Handle.get_id();
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
        l_FunctionInfo.handleType = MeshInfo::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Submesh);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = Submesh::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void MeshInfo::cleanup()
    {
      Low::Util::List<MeshInfo> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_MeshInfo);
      LOW_PROFILE_FREE(type_slots_MeshInfo);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle MeshInfo::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    MeshInfo MeshInfo::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      MeshInfo l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = MeshInfo::TYPE_ID;

      return l_Handle;
    }

    bool MeshInfo::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == MeshInfo::TYPE_ID &&
             check_alive(ms_Slots, MeshInfo::get_capacity());
    }

    uint32_t MeshInfo::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle MeshInfo::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    MeshInfo MeshInfo::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    MeshInfo MeshInfo::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      MeshInfo l_Handle = make(p_Name);
      l_Handle.set_state(get_state());
      l_Handle.set_vertex_count(get_vertex_count());
      l_Handle.set_index_count(get_index_count());
      l_Handle.set_uploaded_vertex_count(get_uploaded_vertex_count());
      l_Handle.set_uploaded_index_count(get_uploaded_index_count());
      l_Handle.set_vertex_start(get_vertex_start());
      l_Handle.set_index_start(get_index_start());
      if (get_submesh().is_alive()) {
        l_Handle.set_submesh(get_submesh());
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    MeshInfo MeshInfo::duplicate(MeshInfo p_Handle,
                                 Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle MeshInfo::_duplicate(Low::Util::Handle p_Handle,
                                           Low::Util::Name p_Name)
    {
      MeshInfo l_MeshInfo = p_Handle.get_id();
      return l_MeshInfo.duplicate(p_Name);
    }

    void MeshInfo::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void MeshInfo::serialize(Low::Util::Handle p_Handle,
                             Low::Util::Yaml::Node &p_Node)
    {
      MeshInfo l_MeshInfo = p_Handle.get_id();
      l_MeshInfo.serialize(p_Node);
    }

    Low::Util::Handle
    MeshInfo::deserialize(Low::Util::Yaml::Node &p_Node,
                          Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    MeshResourceState MeshInfo::get_state() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshInfo, state, MeshResourceState);
    }
    void MeshInfo::set_state(MeshResourceState p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshInfo, state, MeshResourceState) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state
    }

    uint32_t MeshInfo::get_vertex_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_vertex_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshInfo, vertex_count, uint32_t);
    }
    void MeshInfo::set_vertex_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertex_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshInfo, vertex_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_vertex_count
    }

    uint32_t MeshInfo::get_index_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_index_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_index_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshInfo, index_count, uint32_t);
    }
    void MeshInfo::set_index_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_index_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_index_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshInfo, index_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_index_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_index_count
    }

    uint32_t MeshInfo::get_uploaded_vertex_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded_vertex_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded_vertex_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshInfo, uploaded_vertex_count, uint32_t);
    }
    void MeshInfo::set_uploaded_vertex_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded_vertex_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded_vertex_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshInfo, uploaded_vertex_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded_vertex_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded_vertex_count
    }

    uint32_t MeshInfo::get_uploaded_index_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded_index_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded_index_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshInfo, uploaded_index_count, uint32_t);
    }
    void MeshInfo::set_uploaded_index_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded_index_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded_index_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshInfo, uploaded_index_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded_index_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded_index_count
    }

    uint32_t MeshInfo::get_vertex_start() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertex_start
      // LOW_CODEGEN::END::CUSTOM:GETTER_vertex_start

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshInfo, vertex_start, uint32_t);
    }
    void MeshInfo::set_vertex_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertex_start
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertex_start

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshInfo, vertex_start, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertex_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_vertex_start
    }

    uint32_t MeshInfo::get_index_start() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_index_start
      // LOW_CODEGEN::END::CUSTOM:GETTER_index_start

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshInfo, index_start, uint32_t);
    }
    void MeshInfo::set_index_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_index_start
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_index_start

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshInfo, index_start, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_index_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_index_start
    }

    Submesh MeshInfo::get_submesh() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_submesh
      // LOW_CODEGEN::END::CUSTOM:GETTER_submesh

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshInfo, submesh, Submesh);
    }
    void MeshInfo::set_submesh(Submesh p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_submesh
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_submesh

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshInfo, submesh, Submesh) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_submesh
      // LOW_CODEGEN::END::CUSTOM:SETTER_submesh
    }

    Low::Util::Name MeshInfo::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MeshInfo, name, Low::Util::Name);
    }
    void MeshInfo::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MeshInfo, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    MeshInfo MeshInfo::make(Submesh p_Submesh)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      MeshInfo l_MeshInfo = MeshInfo::make(p_Submesh.get_name());
      l_MeshInfo.set_submesh(p_Submesh);

      return l_MeshInfo;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint32_t MeshInfo::create_instance()
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

    void MeshInfo::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(MeshInfoData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(
            &l_NewBuffer[offsetof(MeshInfoData, state) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MeshInfoData, state) * (l_Capacity)],
            l_Capacity * sizeof(MeshResourceState));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshInfoData, vertex_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshInfoData, vertex_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshInfoData, index_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshInfoData, index_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(MeshInfoData,
                                  uploaded_vertex_count) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MeshInfoData, uploaded_vertex_count) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(MeshInfoData,
                                  uploaded_index_count) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MeshInfoData, uploaded_index_count) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshInfoData, vertex_start) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshInfoData, vertex_start) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshInfoData, index_start) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshInfoData, index_start) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshInfoData, submesh) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshInfoData, submesh) *
                          (l_Capacity)],
               l_Capacity * sizeof(Submesh));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(MeshInfoData, name) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MeshInfoData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for MeshInfo from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
