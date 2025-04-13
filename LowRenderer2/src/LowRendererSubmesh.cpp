#include "LowRendererSubmesh.h"

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

    const uint16_t Submesh::TYPE_ID = 50;
    uint32_t Submesh::ms_Capacity = 0u;
    uint8_t *Submesh::ms_Buffer = 0;
    std::shared_mutex Submesh::ms_BufferMutex;
    Low::Util::Instances::Slot *Submesh::ms_Slots = 0;
    Low::Util::List<Submesh> Submesh::ms_LivingInstances =
        Low::Util::List<Submesh>();

    Submesh::Submesh() : Low::Util::Handle(0ull)
    {
    }
    Submesh::Submesh(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Submesh::Submesh(Submesh &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Submesh::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Submesh Submesh::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      Submesh l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Submesh::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Submesh, state,
                              MeshResourceState)) MeshResourceState();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Submesh, meshinfos,
                              Low::Util::List<MeshInfo>))
          Low::Util::List<MeshInfo>();
      ACCESSOR_TYPE_SOA(l_Handle, Submesh, name, Low::Util::Name) =
          Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Submesh::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const Submesh *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void Submesh::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(Submesh));

      initialize_buffer(&ms_Buffer, SubmeshData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_Submesh);
      LOW_PROFILE_ALLOC(type_slots_Submesh);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Submesh);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Submesh::is_alive;
      l_TypeInfo.destroy = &Submesh::destroy;
      l_TypeInfo.serialize = &Submesh::serialize;
      l_TypeInfo.deserialize = &Submesh::deserialize;
      l_TypeInfo.find_by_index = &Submesh::_find_by_index;
      l_TypeInfo.find_by_name = &Submesh::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Submesh::_make;
      l_TypeInfo.duplicate_default = &Submesh::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Submesh::living_instances);
      l_TypeInfo.get_living_count = &Submesh::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(SubmeshData, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            MeshResourceStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Submesh l_Handle = p_Handle.get_id();
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Submesh, state,
                                            MeshResourceState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Submesh l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(MeshResourceState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Submesh l_Handle = p_Handle.get_id();
          *((MeshResourceState *)p_Data) = l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
      {
        // Property: meshinfo_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(meshinfo_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SubmeshData, meshinfo_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Submesh l_Handle = p_Handle.get_id();
          l_Handle.get_meshinfo_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Submesh,
                                            meshinfo_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Submesh l_Handle = p_Handle.get_id();
          l_Handle.set_meshinfo_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Submesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_meshinfo_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: meshinfo_count
      }
      {
        // Property: uploaded_meshinfo_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uploaded_meshinfo_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SubmeshData, uploaded_meshinfo_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Submesh l_Handle = p_Handle.get_id();
          l_Handle.get_uploaded_meshinfo_count();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Submesh, uploaded_meshinfo_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Submesh l_Handle = p_Handle.get_id();
          l_Handle.set_uploaded_meshinfo_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Submesh l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) =
              l_Handle.get_uploaded_meshinfo_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: uploaded_meshinfo_count
      }
      {
        // Property: meshinfos
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(meshinfos);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(SubmeshData, meshinfos);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Submesh l_Handle = p_Handle.get_id();
          l_Handle.get_meshinfos();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Submesh, meshinfos,
              Low::Util::List<MeshInfo>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Submesh l_Handle = p_Handle.get_id();
          l_Handle.set_meshinfos(
              *(Low::Util::List<MeshInfo> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Submesh l_Handle = p_Handle.get_id();
          *((Low::Util::List<MeshInfo> *)p_Data) =
              l_Handle.get_meshinfos();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: meshinfos
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(SubmeshData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Submesh l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Submesh, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Submesh l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Submesh l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Submesh::cleanup()
    {
      Low::Util::List<Submesh> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Submesh);
      LOW_PROFILE_FREE(type_slots_Submesh);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle Submesh::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    Submesh Submesh::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Submesh l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = Submesh::TYPE_ID;

      return l_Handle;
    }

    bool Submesh::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == Submesh::TYPE_ID &&
             check_alive(ms_Slots, Submesh::get_capacity());
    }

    uint32_t Submesh::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle Submesh::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    Submesh Submesh::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    Submesh Submesh::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      Submesh l_Handle = make(p_Name);
      l_Handle.set_state(get_state());
      l_Handle.set_meshinfo_count(get_meshinfo_count());
      l_Handle.set_uploaded_meshinfo_count(
          get_uploaded_meshinfo_count());
      l_Handle.set_meshinfos(get_meshinfos());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    Submesh Submesh::duplicate(Submesh p_Handle,
                               Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle Submesh::_duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name)
    {
      Submesh l_Submesh = p_Handle.get_id();
      return l_Submesh.duplicate(p_Name);
    }

    void Submesh::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Submesh::serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node)
    {
      Submesh l_Submesh = p_Handle.get_id();
      l_Submesh.serialize(p_Node);
    }

    Low::Util::Handle
    Submesh::deserialize(Low::Util::Yaml::Node &p_Node,
                         Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    MeshResourceState Submesh::get_state() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Submesh, state, MeshResourceState);
    }
    void Submesh::set_state(MeshResourceState p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Submesh, state, MeshResourceState) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state
    }

    uint32_t Submesh::get_meshinfo_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_meshinfo_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_meshinfo_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Submesh, meshinfo_count, uint32_t);
    }
    void Submesh::set_meshinfo_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_meshinfo_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_meshinfo_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Submesh, meshinfo_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_meshinfo_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_meshinfo_count
    }

    uint32_t Submesh::get_uploaded_meshinfo_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded_meshinfo_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded_meshinfo_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Submesh, uploaded_meshinfo_count, uint32_t);
    }
    void Submesh::set_uploaded_meshinfo_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded_meshinfo_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded_meshinfo_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Submesh, uploaded_meshinfo_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded_meshinfo_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded_meshinfo_count
    }

    Low::Util::List<MeshInfo> &Submesh::get_meshinfos() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_meshinfos
      // LOW_CODEGEN::END::CUSTOM:GETTER_meshinfos

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Submesh, meshinfos, Low::Util::List<MeshInfo>);
    }
    void Submesh::set_meshinfos(Low::Util::List<MeshInfo> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_meshinfos
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_meshinfos

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Submesh, meshinfos, Low::Util::List<MeshInfo>) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_meshinfos
      // LOW_CODEGEN::END::CUSTOM:SETTER_meshinfos
    }

    Low::Util::Name Submesh::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Submesh, name, Low::Util::Name);
    }
    void Submesh::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Submesh, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    uint32_t Submesh::create_instance()
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

    void Submesh::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(SubmeshData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(
            &l_NewBuffer[offsetof(SubmeshData, state) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(SubmeshData, state) * (l_Capacity)],
            l_Capacity * sizeof(MeshResourceState));
      }
      {
        memcpy(&l_NewBuffer[offsetof(SubmeshData, meshinfo_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SubmeshData, meshinfo_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(SubmeshData,
                                     uploaded_meshinfo_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SubmeshData,
                                   uploaded_meshinfo_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          Submesh i_Submesh = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(SubmeshData, meshinfos) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Low::Util::List<MeshInfo>))])
              Low::Util::List<MeshInfo>();
          *i_ValPtr = ACCESSOR_TYPE_SOA(i_Submesh, Submesh, meshinfos,
                                        Low::Util::List<MeshInfo>);
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(SubmeshData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SubmeshData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for Submesh from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
