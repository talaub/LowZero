#include "LowRendererMesh.h"

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

    const uint16_t Mesh::TYPE_ID = 15;
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

      ACCESSOR_TYPE_SOA(l_Handle, Mesh, name, Low::Util::Name) =
          Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Mesh::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

      // LOW_CODEGEN::END::CUSTOM:DESTROY

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
          Low::Util::Config::get_capacity(N(LowRenderer), N(Mesh));

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
        // Property: vertex_buffer_start
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertex_buffer_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshData, vertex_buffer_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.get_vertex_buffer_start();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Mesh, vertex_buffer_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_vertex_buffer_start(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_vertex_buffer_start();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: vertex_buffer_start
      }
      {
        // Property: vertex_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertex_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshData, vertex_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.get_vertex_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh,
                                            vertex_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_vertex_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_vertex_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: vertex_count
      }
      {
        // Property: index_buffer_start
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(index_buffer_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshData, index_buffer_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.get_index_buffer_start();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Mesh, index_buffer_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_index_buffer_start(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_index_buffer_start();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: index_buffer_start
      }
      {
        // Property: index_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(index_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MeshData, index_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.get_index_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh,
                                            index_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_index_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_index_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: index_count
      }
      {
        // Property: vertexweight_buffer_start
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertexweight_buffer_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshData, vertexweight_buffer_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.get_vertexweight_buffer_start();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Mesh, vertexweight_buffer_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_vertexweight_buffer_start(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) =
              l_Handle.get_vertexweight_buffer_start();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: vertexweight_buffer_start
      }
      {
        // Property: vertexweight_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertexweight_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshData, vertexweight_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.get_vertexweight_count();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Mesh, vertexweight_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_vertexweight_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_vertexweight_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: vertexweight_count
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
      l_Handle.set_vertex_buffer_start(get_vertex_buffer_start());
      l_Handle.set_vertex_count(get_vertex_count());
      l_Handle.set_index_buffer_start(get_index_buffer_start());
      l_Handle.set_index_count(get_index_count());
      l_Handle.set_vertexweight_buffer_start(
          get_vertexweight_buffer_start());
      l_Handle.set_vertexweight_count(get_vertexweight_count());

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

      p_Node["vertex_buffer_start"] = get_vertex_buffer_start();
      p_Node["vertex_count"] = get_vertex_count();
      p_Node["index_buffer_start"] = get_index_buffer_start();
      p_Node["index_count"] = get_index_count();
      p_Node["vertexweight_buffer_start"] =
          get_vertexweight_buffer_start();
      p_Node["vertexweight_count"] = get_vertexweight_count();
      p_Node["name"] = get_name().c_str();

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
      Mesh l_Handle = Mesh::make(N(Mesh));

      if (p_Node["vertex_buffer_start"]) {
        l_Handle.set_vertex_buffer_start(
            p_Node["vertex_buffer_start"].as<uint32_t>());
      }
      if (p_Node["vertex_count"]) {
        l_Handle.set_vertex_count(
            p_Node["vertex_count"].as<uint32_t>());
      }
      if (p_Node["index_buffer_start"]) {
        l_Handle.set_index_buffer_start(
            p_Node["index_buffer_start"].as<uint32_t>());
      }
      if (p_Node["index_count"]) {
        l_Handle.set_index_count(
            p_Node["index_count"].as<uint32_t>());
      }
      if (p_Node["vertexweight_buffer_start"]) {
        l_Handle.set_vertexweight_buffer_start(
            p_Node["vertexweight_buffer_start"].as<uint32_t>());
      }
      if (p_Node["vertexweight_count"]) {
        l_Handle.set_vertexweight_count(
            p_Node["vertexweight_count"].as<uint32_t>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    uint32_t Mesh::get_vertex_buffer_start() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertex_buffer_start

      // LOW_CODEGEN::END::CUSTOM:GETTER_vertex_buffer_start

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Mesh, vertex_buffer_start, uint32_t);
    }
    void Mesh::set_vertex_buffer_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertex_buffer_start

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertex_buffer_start

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Mesh, vertex_buffer_start, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertex_buffer_start

      // LOW_CODEGEN::END::CUSTOM:SETTER_vertex_buffer_start
    }

    uint32_t Mesh::get_vertex_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertex_count

      // LOW_CODEGEN::END::CUSTOM:GETTER_vertex_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Mesh, vertex_count, uint32_t);
    }
    void Mesh::set_vertex_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertex_count

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertex_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Mesh, vertex_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertex_count

      // LOW_CODEGEN::END::CUSTOM:SETTER_vertex_count
    }

    uint32_t Mesh::get_index_buffer_start() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_index_buffer_start

      // LOW_CODEGEN::END::CUSTOM:GETTER_index_buffer_start

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Mesh, index_buffer_start, uint32_t);
    }
    void Mesh::set_index_buffer_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_index_buffer_start

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_index_buffer_start

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Mesh, index_buffer_start, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_index_buffer_start

      // LOW_CODEGEN::END::CUSTOM:SETTER_index_buffer_start
    }

    uint32_t Mesh::get_index_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_index_count

      // LOW_CODEGEN::END::CUSTOM:GETTER_index_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Mesh, index_count, uint32_t);
    }
    void Mesh::set_index_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_index_count

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_index_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Mesh, index_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_index_count

      // LOW_CODEGEN::END::CUSTOM:SETTER_index_count
    }

    uint32_t Mesh::get_vertexweight_buffer_start() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertexweight_buffer_start

      // LOW_CODEGEN::END::CUSTOM:GETTER_vertexweight_buffer_start

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Mesh, vertexweight_buffer_start, uint32_t);
    }
    void Mesh::set_vertexweight_buffer_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertexweight_buffer_start

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertexweight_buffer_start

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Mesh, vertexweight_buffer_start, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertexweight_buffer_start

      // LOW_CODEGEN::END::CUSTOM:SETTER_vertexweight_buffer_start
    }

    uint32_t Mesh::get_vertexweight_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertexweight_count

      // LOW_CODEGEN::END::CUSTOM:GETTER_vertexweight_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Mesh, vertexweight_count, uint32_t);
    }
    void Mesh::set_vertexweight_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertexweight_count

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertexweight_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Mesh, vertexweight_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertexweight_count

      // LOW_CODEGEN::END::CUSTOM:SETTER_vertexweight_count
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
        memcpy(&l_NewBuffer[offsetof(MeshData, vertex_buffer_start) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshData, vertex_buffer_start) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshData, vertex_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshData, vertex_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshData, index_buffer_start) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshData, index_buffer_start) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshData, index_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshData, index_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(MeshData,
                                  vertexweight_buffer_start) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MeshData, vertexweight_buffer_start) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshData, vertexweight_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshData, vertexweight_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
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
